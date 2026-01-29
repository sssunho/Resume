using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.AI;

#if !MA_COLLISION
public class UnitMovement
{
    public enum EAvoidancePriority
    {
        Stationary = 1,
        Player = 2,
        Boss   = Stationary,
        Friend = 5,
        Others = 6,
        MAX = 99,
    }

    enum DrawUpdateType
    {
        DrawFollowAgent,
        AgentFollowDraw,
    }

    class AddVelocityInfo
    {
        public int Priority;
        public Vector3 Velocity;
        public float PastTime;
        public float Duration;

        public void Update()
        {
            if (PastTime >= Duration) return;
            PastTime += Time.deltaTime;
        }
    }

    GameModelDraw _owner;
    NavMeshAgent _agent;
    bool _checkCollision;
    DrawUpdateType _drawUpdateType;
    Vector3 _drawVelocity; // agent 끄고 움직일 때 사용
    List<AddVelocityInfo> _addVelocities = new();

    public NavMeshAgent Agent => _agent;
    public Vector3 AgentPosition
    {
        get => _agent.nextPosition;
    }

    public Vector3 Velocity
    {
        get => _drawUpdateType == DrawUpdateType.DrawFollowAgent ? _agent.velocity : _drawVelocity;
        set
        {
            if (_drawUpdateType == DrawUpdateType.DrawFollowAgent)
                _agent.velocity = value;
            else
                _drawVelocity = value;
        }
    }

    public float CurrentSqrSpeed
    {
        get => _agent.velocity.sqrMagnitude;
    }

    public float CurrentSpeed
    {
        get => _agent.velocity.magnitude;
    }

    public float MoveSpeedPerSec
    {
        get => _agent.speed;
        set => _agent.speed = value;
    }

    public Vector3 DesiredVelocity
    {
        get => _agent.desiredVelocity;
    }

    public float Radius
    {
        get => _agent.radius;
        set
        {
            _agent.radius = value;
            if (_agent.radius <= 0.05f) _agent.radius = 0.05f;
        }
    }

    public bool FollowPath
    {
        get => _agent.isStopped;
        set
        {
            if (_agent.isOnNavMesh == false) return;
            _agent.isStopped = !value;
            if (value == false) _agent.ResetPath();
        }
    }

    public bool CheckCollision
    {
        get => _checkCollision;
        set
        {
            if (_checkCollision == value) return;
            _checkCollision = value;
            _drawUpdateType = value ? DrawUpdateType.DrawFollowAgent : DrawUpdateType.AgentFollowDraw;
            _agent.enabled  = value;
            _agent.velocity = Vector3.zero;
            _drawVelocity   = Vector3.zero;
        }
    }

    public float YOffset
    {
        get => _agent.baseOffset * _owner.GetScale();
        set => _agent.baseOffset = value / _owner.GetScale();
    }

    public bool MoveTo(Vector3 pos)
    {
        if (_agent.isOnNavMesh == false || _agent.enabled == false) return false;
        var res = _agent.SetDestination(pos);
        return res;
    }

    public bool MoveTo(MANPCUnit unit)
    {
        return MoveTo(unit.GetDrawPosition());
    }
    
    public bool Warp(Vector3 pos) => _agent.Warp(pos);

    public void ResetPath()
    {
        if (_agent.isOnNavMesh == false) return;
        _agent.ResetPath();
    }

    public void Init(GameModelDraw draw)
    {
        if (null == draw) return;
        var obj = draw.GetOwnerModel();
        _agent = obj.GetComponent<NavMeshAgent>();
        if (_agent == null) _agent = obj.AddComponent<NavMeshAgent>();
        _owner = draw;

        MoveSpeedPerSec = 5f;
        _agent.acceleration = 100f;
        _agent.autoBraking = false;
        YOffset = _owner.GetLinkModelInfo().YOffset;

        _agent.updatePosition = false;
        _agent.updateRotation = false;
        _agent.obstacleAvoidanceType = ObstacleAvoidanceType.LowQualityObstacleAvoidance;

        CheckCollision = true;

    }

    float _vibrationPower;
    float _vibrationDuration;
    float _vibrationTime;
    float _vibrationSpeed = 20f * Mathf.PI;

    public void SetVibration(float power, float time)
    {
        _vibrationPower = power;
        _vibrationDuration = time;
        _vibrationTime = 0f;
    }

    public void Update()
    {
        if (null == _owner) return;

        if(_drawUpdateType == DrawUpdateType.DrawFollowAgent)
        {
            var pos = _agent.nextPosition;
            if (_vibrationDuration > 0f)
            {
                float power = Mathf.Lerp(_vibrationPower, 0f, _vibrationTime / _vibrationDuration);
                _vibrationTime += Time.deltaTime;
                if (_vibrationTime < _vibrationDuration)
                {
                    pos += _owner.GetOwnerModel().transform.right * (power * Mathf.Sin(_vibrationSpeed * _vibrationTime));
                }
                else
                {
                    _vibrationPower = _vibrationTime = _vibrationDuration = 0f;
                }
            }
            _owner.SetPosition(pos);
        }
        else
        {
            var delta = _drawVelocity * Time.deltaTime;
            var src = _owner.GetPosition() + delta + Vector3.up * 2f;

            if (Physics.Raycast(src, Vector3.down, out var hit, 12345f, 1 << LayerMask.NameToLayer("Map")))
            {
                if (NavMesh.SamplePosition(hit.point, out var navhit, 0.3f, NavMesh.AllAreas))
                {
                    _owner.SetPosition(navhit.position);
                }
            }
            _drawVelocity = Vector3.zero;
        }
    }

    public void LateUpdate(MANPCUnit unit)
    {
        int priority = 0;
        var vel = default(Vector3);
        var basevel = _agent.desiredVelocity;
        if (unit.CurrentTarget != null && (unit.CharacterType == ECharacterType.Boss || unit.CharacterType == ECharacterType.Player) && unit.UnitStateType == EUnitState.Move && unit.HasExternalInput == false && _agent.path.corners.Length < 3)
        {
            basevel = (unit.CurrentTarget.GetDrawPosition() - unit.GetDrawPosition()).normalized * MoveSpeedPerSec;
        }
        foreach(var info in _addVelocities)
        {
            vel += info.Velocity * info.Priority;
            priority += info.Priority;
            info.Update();
        }
        if(priority != 0) vel /= priority;
        if (_drawUpdateType == DrawUpdateType.DrawFollowAgent)
            _agent.velocity = basevel + vel;
        else
            _drawVelocity = vel;
        _addVelocities.RemoveAll(p => p.PastTime >= p.Duration);
    }

    public void AddVelocity(Vector3 vel, float duration = 0f, int priority = 1)
    {
        if (priority < 0) priority = 0;
        _addVelocities.Add(new()
        {
            Duration = duration,
            Velocity = vel,
            Priority = priority,
        });
    }

    public bool ReachedDestinationOrGaveUp()
    {
        if (_checkCollision == false) return true;
        return _agent.pathPending == false && _agent.remainingDistance <= _agent.stoppingDistance && (_agent.hasPath == false || _agent.velocity.sqrMagnitude == 0f);
    }
}
#endif