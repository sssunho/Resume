using System;
using System.Numerics;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Vector2 = UnityEngine.Vector2;
using Vector3 = UnityEngine.Vector3;
using Quaternion = UnityEngine.Quaternion;
using Random = UnityEngine.Random;

[System.Serializable]
public class ProjectileCurve
{
    public const int CURVE_POINT_COUNT = 2;

    public Eff_Projectile.CurveType CurveType = Eff_Projectile.CurveType.Straight;
    public Vector3 StartPoint = Vector3.zero;
    public Vector3 EndPoint = new Vector3(0, 0, 15f);
    public Vector3[] CurvePoints = new Vector3[CURVE_POINT_COUNT] { 5f * Vector3.forward, 10f * Vector3.forward };
    public AnimationCurve TimeCurve = AnimationCurve.Linear(0f, 1f, 1f, 1f);
}

public class Eff_Projectile : Eff_Base
{
    public enum LookType
    {
        None,
        Tangent,
        Target,
        StraightUp,
    }

    public enum FollowType
    {
        None,
        Position,
        Target,
    }

    public enum CurveType
    {
        Straight,
        Bazier,
        //PID,
        RandomBazier,
    }

    public List<ProjectileCurve> Curves = new() { new() };

    public int CurrentCurveID
    {
        get => CurveID;
        set
        {
            CurveID = Curves.Count == 0 ? 0 : Mathf.Clamp(value, 0, Curves.Count - 1);
        }
    }

    public ProjectileCurve CurrentCurve
    {
        get => CurveID >= Curves.Count ? null : Curves[CurveID];
    }

    protected MANPCUnit _skillUseUnit;
    protected SkillInfo _skillInfo;
    protected HashSet<int> _skillHitUnits = new();
    protected Quaternion _initialRotation;
    protected int _targetUnitUID;
    protected TrailRenderer _trail;

    public int CurveID;
    public Vector3 TargetPos;
    public Vector3 DesiredTargetPos => GetDestination();

    public Vector3 StartPos;
    public float Duration;
    public float Delay;
    public float PastTime;
    public float NormalizedTime => (PastTime - Delay) / Duration;

    public string HitEffect;
    public bool Terminated { get; protected set; }

    public LookType LookMode = LookType.Tangent;
    public FollowType FollowMode = FollowType.Target;

    public bool ExplodeOnEndOfDuration = true;
    public bool bHit;
    public bool UseDefaultSpeed;
    public float Speed = 10f;

    protected BigInteger[] _unitStats;
    protected Vector3 _prevPosition;
    float _trailUpdateInterval = 0.05f;
    float _trailUpdateTime = 0f;
    int _actionCount;

    RaycastHit[] _hits = new RaycastHit[5];

    public MANPCUnit TargetUnit
    {
        get
        {
            var dic = BattleSceneManager.Instance.GetBattleScene()?.GetFindUnitDics(_skillUseUnit);
            if (null == dic) return null;
            dic.TryGetValue(_targetUnitUID, out var target);
            return target;
        }
    }

    #region MonoBehaviour Impelements

    void Awake()
    {
        var comp = GetComponent<Eff_DestroyTime>();

        if (comp == null)
        {
            comp = gameObject.AddComponent<Eff_DestroyTime>();
        }
        comp.enabled = false;

        _trail = GetComponentInChildren<TrailRenderer>();
    }

    protected virtual void Update()
    {
        PastTime += Time.deltaTime;
        UpdateTarget();
        UpdateProjectileMovement();
        UpdateProjectileHit();
        UpdateProjectileTerminate();
        _prevPosition = transform.position;
    }

    protected virtual void OnEnable()
    {
        if (Curves.Count == 0) Curves.Add(new());

        if (CurrentCurve.CurveType == CurveType.RandomBazier)
        {
            var delta = new Vector2(Random.Range(5f, 15f), Random.Range(-0.5f, 10f));
            if (Random.value < .5f) delta.x *= -1f;
            float z = Random.Range(-3f, 3f);
            CurrentCurve.CurvePoints[0] = new Vector3(delta.x, delta.y, 5f + z);
            CurrentCurve.CurvePoints[1] = new Vector3(delta.x, delta.y, 10f + z);
        }

        bHit = false;
        StartPos = transform.position;
        _prevPosition = transform.position;
        _initialRotation = transform.rotation;
        PastTime = 0f;
    }

    #endregion

    public override void ResetEffect()
    {
        base.ResetEffect();

        var comp = GetComponent<Eff_DestroyTime>();

        if (comp != null)
        {
            comp.enabled = false;
        }

        _skillHitUnits.Clear();
        _skillInfo = null;
        _skillUseUnit = null;
        _targetUnitUID = 0;
        PastTime = 0f;
    }

    protected virtual void OnDurationEnd()
    {
        if (ExplodeOnEndOfDuration && bHit == false) DamageInArea();

        Terminate();
    }

    protected virtual void OnProjectileHit()
    {
        if (Terminated == true) return;

        bHit = true;
        DamageInArea();
        Terminate();
    }

    protected virtual void UpdateTarget()
    {
        if (FollowMode == FollowType.Target)
        {
            if (TargetUnit != null)
            {
                TargetPos = TargetUnit.GetDrawPosition() + TargetUnit.GetDraw().UnitCollider.center;
            }
            else
            {
                FollowMode = FollowType.Position;
            }
        }
    }

    protected virtual void UpdateProjectileMovement()
    {
        if (Terminated) return;
        if (PastTime >= Delay)
        {
            if (CurrentCurve.CurveType == CurveType.Straight)
            {
                var pos = Vector3.Lerp(StartPos, DesiredTargetPos, NormalizedTime);
                transform.LookAt(pos);
                transform.position = pos;
            }
            else if (CurrentCurve.CurveType == CurveType.Bazier || CurrentCurve.CurveType == CurveType.RandomBazier)
            {
                var timescale = CurrentCurve.TimeCurve.Evaluate(NormalizedTime);
                var curve = Transform(CurrentCurve, StartPos, DesiredTargetPos);
                var pos = GetBazierPointAtTime(curve, Mathf.Clamp01(NormalizedTime * timescale));
                UpdateRotation(curve, NormalizedTime * timescale);
                transform.position = pos;
            }
        }
    }

    void UpdateProjectileHit()
    {
        if (null == BattleSceneManager.Instance.GetBattleScene() || Terminated || bHit == true) return;

        if (FollowMode == FollowType.Target)
        {
            var direction = transform.position - _prevPosition;
            if (direction.sqrMagnitude < 0.01) return;
            var count = Physics.RaycastNonAlloc(_prevPosition, direction, _hits, direction.magnitude, ConstValue.LAYER_CHARACTER);
            var dic = BattleSceneManager.Instance.GetBattleScene().GetFindUnitDics(_skillUseUnit);
            float mindist = float.MaxValue;
            MANPCUnit firstHitTarget = null;
            int minindex = count + 1;
            for (int i = 0; i < count; i++)
            {
                var hit = _hits[i];
                int id = int.Parse(hit.collider.gameObject.name.Split('_')[1]);
                if (dic.TryGetValue(id, out var target) == false) continue;
                float dist = Vector3.SqrMagnitude(_prevPosition - target.GetDrawPosition());
                if (dist < mindist)
                {
                    mindist = dist;
                    firstHitTarget = target;
                    minindex = i;
                }
            }
            if (null != firstHitTarget)
            {
                transform.position = _hits[minindex].point;
                OnProjectileHit();
            }
        }
        else if (FollowMode == FollowType.Position)
        {
            if (IsPointBetweenTwoPoint(GetDestination(), transform.position, _prevPosition))
            {
                transform.position = GetDestination();
                OnProjectileHit();
            }    
        }
    }

    void UpdateProjectileTerminate()
    {
        if (NormalizedTime > 1f && Terminated == false)
        {
            OnDurationEnd();
        }
    }

    void UpdateRotation(ProjectileCurve curve, float time)
    {
        switch (LookMode)
        {
            case LookType.Tangent:
                if (CurrentCurve.CurveType == CurveType.Straight)
                {
                    transform.LookAt(DesiredTargetPos);
                }
                else
                {
                    transform.rotation = Quaternion.LookRotation(GetBazierTangentAtTime(curve, time));
                }
                break;

            case LookType.Target:
                transform.LookAt(DesiredTargetPos);
                break;
        }
    }

    protected virtual void OnProjectileTerminate()
    {

    }

    protected void DamageInArea()
    {
        bHit = true;
        if (null == _skillInfo) return;
        var pos = transform.position;
        pos.y = 0;
        var rotation = MAUtil.AbsRot(transform.rotation.eulerAngles.y);
        var targets = BattleSceneManager.Instance.GetBattleScene()?.GetFindUnitDics(_skillUseUnit).Values;
        var units = _skillInfo.GetSkillHitUnit(targets, _skillInfo.SkillRangeData, pos, rotation);
        if (null == units) 
            return;
        foreach (var unit in units)
        {
            if (_skillHitUnits.Contains(unit.GetUnitUID()) || unit.UnitType == EUnitType.Friend) continue;
            var damage = _skillInfo.GetDamage(_skillUseUnit, transform.position, false);
            if (_actionCount > 0) damage.ScaledValue.Multiply(1, _actionCount);
            damage.EnemyHitCount = units.Count;
            unit.PlayHit(HitEffect, null, damage);
            _skillHitUnits.Add(unit.GetUnitUID());
        }
    }

    #region Math

    bool IsPointBetweenTwoPoint(Vector3 target, Vector3 linep1, Vector3 linep2, float threshold = 0.75f)
    {
        var target_p1 = linep1 - target;
        var direction = linep1 - linep2;
        var sqrdh = Vector3.Cross(target_p1, direction).sqrMagnitude / direction.sqrMagnitude;
        var d1 = Mathf.Sqrt(Vector3.SqrMagnitude(target_p1) - sqrdh);
        var d2 = Vector3.Distance(linep1, linep2);

        return d2 > d1 && sqrdh < threshold;
    }

    protected Vector3 GetBazierPointAtTime(ProjectileCurve curve, float normalizedTime)
    {
        float u = 1f - normalizedTime;
        float tt = normalizedTime * normalizedTime;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * normalizedTime;

        Vector3 p = uuu * curve.StartPoint;
        p += 3 * uu * normalizedTime * curve.CurvePoints[0]; //second term  
        p += 3 * u * tt * curve.CurvePoints[1]; //third term  
        p += ttt * curve.EndPoint; //fourth term  

        return p;
    }

    protected Vector3 GetBazierTangentAtTime(ProjectileCurve curve, float normalizedTime)
    {
        float u = 1f - normalizedTime;
        float tt = normalizedTime * normalizedTime;
        float uu = u * u;

        Vector3 t = 3 * uu * (curve.CurvePoints[0] - curve.StartPoint);
        t += 6 * u * normalizedTime * (curve.CurvePoints[1] - curve.CurvePoints[0]);
        t += 3 * tt * (curve.EndPoint - curve.CurvePoints[1]);

        return t;
    }

    protected ProjectileCurve Transform(ProjectileCurve curve, Vector3 startpos, Vector3 endpos)
    {
        var transformed = new ProjectileCurve();
        var rel = endpos - startpos;
        var look = rel.sqrMagnitude > 1e-3f ? Quaternion.LookRotation(endpos - startpos) : Quaternion.identity;
        var ratio = (startpos - endpos).magnitude / (curve.StartPoint - curve.EndPoint).magnitude;
        transformed.StartPoint = startpos;
        transformed.EndPoint = endpos;
        transformed.CurvePoints[0] = startpos + look * (curve.CurvePoints[0] - curve.StartPoint) * ratio;
        transformed.CurvePoints[1] = startpos + look * (curve.CurvePoints[1] - curve.StartPoint) * ratio;
        return transformed;
    }

    #endregion

    #region Public methods

    public void Create(MANPCUnit skilluser, SkillInfo skillinfo, Vector3 targetPosition, int linenum, string hiteffect)
    {
        FollowMode = FollowType.Position;
        _skillUseUnit = skilluser;
        _skillInfo = skillinfo;
        HitEffect = hiteffect;
        CurrentCurveID = linenum;
        StartPos = transform.position;
        _initialRotation = transform.rotation;
        Terminated = false;
        TargetPos = targetPosition;
    }

    public void Create(MANPCUnit skilluser, SkillInfo skillinfo, MANPCUnit targetUnit, int linenum, string hiteffect)
    {
        FollowMode = FollowType.Target;
        _skillUseUnit = skilluser;
        _skillInfo = skillinfo;
        HitEffect = hiteffect;
        CurrentCurveID = linenum;
        StartPos = transform.position;
        _initialRotation = transform.rotation;
        _targetUnitUID = targetUnit.GetUnitUID();
        TargetPos = GetDestination();
        Terminated = false;
    }

    public void SetTarget(MANPCUnit unit)
    {
        _targetUnitUID = unit.GetUnitUID();
        TargetPos = unit.GetDrawPosition();
        FollowMode = FollowType.Target;
        UpdateRotation(Transform(CurrentCurve, transform.position, TargetPos), 0f);
    }

    public void SetDestiation(Vector3 pos)
    {
        _targetUnitUID = -1;
        TargetPos = pos;
        FollowMode = FollowType.Position;
        UpdateRotation(Transform(CurrentCurve, transform.position, TargetPos), 0f);
    }

    public void Terminate(float destroytime = 0.05f)
    {
        if (TargetUnit != null && TargetUnit.IsPossiblePlayEffect(HitEffect))
        {
            var bound = TargetUnit.GetDrawBound();
            transform.position = bound.ClosestPoint(transform.position);
            EffectManager.Instance.PlayEffect(HitEffect, transform.position, 0f);
        }
        else if (TargetUnit == null)
        {
            transform.position = DesiredTargetPos;
            EffectManager.Instance.PlayEffect(HitEffect, transform.position, 0f);
        }

        var comp = GetComponent<Eff_DestroyTime>();
        if (comp != null)
        {
            comp.enabled = true;
        }
        else
        {
            Destroy(gameObject, destroytime);
        }
        Terminated = true;

        OnProjectileTerminate();
    }

    public Vector3 GetDestination()
    {
        switch (FollowMode)
        {
            case FollowType.Position:
                return TargetPos;
            case FollowType.Target:
                if (TargetUnit != null) TargetPos = TargetUnit.GetDrawPosition() + TargetUnit.GetDraw().UnitCollider.center;
                return TargetPos;

            default:
                return StartPos + _initialRotation * (CurrentCurve.EndPoint - CurrentCurve.StartPoint);
        }
    }

    public void SetSpeed(float speed)
    {
        Duration = Vector3.Distance(StartPos, TargetPos) / speed;
        Speed = speed;
    }

    public void SetActionCount(int count)
    {
        _actionCount = count;
    }

    #endregion

#if UNITY_EDITOR
    #region Debug

    void OnDrawGizmosSelected()
    {
        if (null == CurrentCurve) return;

        if (Application.isPlaying)
        {
            if (CurrentCurve.CurveType == CurveType.Straight)
            {
                Debug.DrawLine(StartPos, DesiredTargetPos, Color.red);
            }
            else
            {
                var t = Transform(CurrentCurve, StartPos, DesiredTargetPos);
                DrawCurve(t, Color.red);
            }
        }
        else
        {
            if (CurrentCurve.CurveType == CurveType.Straight)
            {
                Debug.DrawLine(transform.position, transform.position + transform.rotation * (CurrentCurve.EndPoint - CurrentCurve.StartPoint), Color.white);
            }
            else
            {
                var end = FollowMode == FollowType.Position ? TargetPos : transform.position + transform.rotation * (CurrentCurve.EndPoint - CurrentCurve.StartPoint);
                var t = Transform(CurrentCurve, transform.position, end);
                DrawCurve(t, Color.white);
            }
        }
    }

    void DrawCurve(ProjectileCurve curve, Color color)
    {
        if (null == curve) return;
        var p = curve.StartPoint;
        for (int i = 0; i <= 30; i++)
        {
            var pp = GetBazierPointAtTime(curve, i / 30f);
            if (i % 3 == 0)
            {
                Debug.DrawLine(p, p + GetBazierTangentAtTime(curve, i / 30f).normalized * 3f, Color.green);
            }
            Debug.DrawLine(p, pp, color);
            p = pp;
        }
    }

    #endregion
#endif
}