"""Tune the racing line's PlanParams offline: Optuna (TPE) searches the parameters below, each trial
planning a set of routes with navplan (the plugin's own planner and the car's limits from
src/red4ext/Navigation/Vehicle.hpp) and scoring the lines against the current defaults. Routes are
split into a tuning set and a held-out set; the report gives both, and the best parameters as
NAVPLAN_PARAMS to try in game. Nothing ships from here: every result gets an in-game A/B with a
video first (the offline score does not see the follower).

    python tools/navplan/tune.py --trials 300 --out <dir> [--vehicle shion] [--routes 30] [--jobs 14]
    python tools/navplan/tune.py --out <dir> --report        the study so far

Score per route (lower is better; 0 = the defaults), averaged over the routes:
  speed lost in dips (dips.py) per 10 m/s; the bend's rate of change (jerk) and the sharpest bend,
  relative; the share of the line within 3 m of blocked space, per 2 %; the time, relative; the mean
  height over the defaults' line per 20 m (over rather than under is fine, a needless climb is
  not); 10 for a route that loses its racing line, -2 for one that gains a line the defaults lack;
  plan time only past twice the defaults' (absolute times depend on how many plans run at once).
"""

import argparse
import json
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import measure  # noqa: E402
from measure import cmake_navmap, routes, run_all, score, vehicle_spec  # noqa: E402

# name: (low, high, log, int); defaults come from Planner.hpp (the baseline trial uses none of these)
SPACE = {
    "lineCorridorMargin": (3.0, 10.0, False, False),
    "lineCorridorMax": (20.0, 60.0, False, False),
    "lineCurvatureRate": (0.1, 2.0, True, False),
    "lineTurnRounds": (0, 5, False, True),
    "lineTurnWeightMax": (5.0, 100.0, True, False),
    "lineVerticalWeight": (0.5, 1.55, False, False),  # stable while lineStep (0.08) x it < 0.125
    "lineSpread": (10.0, 60.0, False, False),
    "linePush": (0.02, 0.3, True, False),
    "lineClearance": (3.0, 10.0, False, False),
    "lineHeightWavelength": (300.0, 2000.0, True, False),
    "lineHeightFree": (10.0, 60.0, False, False),
    "lineHeightScale": (10.0, 60.0, False, False),
    "lineOverGain": (0.1, 0.5, False, False),
    "turnCost": (0.0, 10.0, False, False),
}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--trials", type=int, default=200)
    ap.add_argument("--routes", type=int, default=30, help="helipad routes besides the 5 benchmarks")
    ap.add_argument("--vehicle", default="shion")
    ap.add_argument("--navmap")
    ap.add_argument("--navplan", help="navplan.exe to run (default build/navplan.exe)")
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 4) - 2))
    ap.add_argument("--report", action="store_true")
    ap.add_argument("--seed", help="json of parameters to try first (e.g. an earlier study's best)")
    args = ap.parse_args()
    import optuna

    measure.set_navplan(args.navplan)
    navmap = args.navmap or cmake_navmap()
    vehicle = vehicle_spec(args.vehicle)
    rts = routes(args.routes)
    held = set(r[0] for i, r in enumerate(rts) if i % 10 in (3, 6, 9))  # 30% held out
    os.makedirs(args.out, exist_ok=True)
    base_file = os.path.join(args.out, "baseline.json")
    if os.path.exists(base_file):
        with open(base_file) as f:
            base = json.load(f)
    else:
        print(f"baseline: {len(rts)} routes with the defaults", flush=True)
        base = dict(zip([r[0] for r in rts], run_all(navmap, vehicle, rts, {}, os.path.join(args.out, "baseline"), args.jobs)))
        with open(base_file, "w") as f:
            json.dump(base, f, indent=1)
    lost_lines = [n for n, m in base.items() if not m or not m["ok"]]
    print(f"baseline: {len(lost_lines)} of {len(base)} routes without a racing line {lost_lines}", flush=True)

    def evaluate(params, subset, tag):
        res = run_all(navmap, vehicle, subset, params, os.path.join(args.out, tag), args.jobs)
        return [score(m, base.get(r[0])) for r, m in zip(subset, res)], res

    tune_set = [r for r in rts if r[0] not in held]
    test_set = [r for r in rts if r[0] in held]
    study = optuna.create_study(study_name="racing_line", storage=f"sqlite:///{os.path.join(args.out, 'study.db')}",
                                load_if_exists=True, direction="minimize",
                                sampler=optuna.samplers.TPESampler(seed=1, multivariate=True))

    if args.seed and not study.trials:
        with open(args.seed) as f:
            study.enqueue_trial(json.load(f))

    def objective(trial):
        params = {}
        for k, (lo, hi, log, is_int) in SPACE.items():
            params[k] = trial.suggest_int(k, lo, hi) if is_int else trial.suggest_float(k, lo, hi, log=log)
        scores, res = evaluate(params, tune_set, "trial")
        trial.set_user_attr("fails", sum(1 for m in res if not m or not m["ok"]))
        return float(np.mean(scores))

    if not args.report and args.trials > 0:
        optuna.logging.set_verbosity(optuna.logging.WARNING)
        study.optimize(objective, n_trials=args.trials,
                       callbacks=[lambda s, t: print(f"trial {t.number}: {t.value:.3f} (best {s.best_value:.3f}, "
                                                     f"{t.user_attrs.get('fails')} lines lost)", flush=True)])
    if not any(t.state == optuna.trial.TrialState.COMPLETE for t in study.trials):
        return
    best = study.best_trial
    print(f"\nbest trial {best.number}: tuning score {best.value:.3f} (0 = the defaults, lower is better)")
    test_scores, test_res = evaluate(best.params, test_set, "best_test")
    tune_scores, tune_res = evaluate(best.params, tune_set, "best_tune")
    print(f"held out ({len(test_set)} routes): {np.mean(test_scores):.3f}")
    print(f"{'route':10s} {'score':>6s} {'lost':>11s} {'jerk':>11s} {'sharpest':>11s} {'tight%':>11s} {'time':>11s}")
    for r, m, s in list(zip(tune_set, tune_res, tune_scores)) + list(zip(test_set, test_res, test_scores)):
        b = base.get(r[0])
        if not m or not b:
            print(f"{r[0]:10s} {s:6.2f}  (no line)")
            continue

        def pair(key, fmt="{:.1f}"):
            return f"{fmt.format(b[key])}>{fmt.format(m[key])}"
        mark = "*" if r[0] in held else " "
        print(f"{r[0]:9s}{mark} {s:6.2f} {pair('lost', '{:.0f}'):>11s} {pair('jerk', '{:.2f}'):>11s} {pair('sharpest'):>11s} "
              f"{pair('tight'):>11s} {pair('time'):>11s}")
    print("(* held out)")
    print("\nNAVPLAN_PARAMS=" + ",".join(f"{k}={v:.4g}" if isinstance(v, float) else f"{k}={v}" for k, v in best.params.items()))


if __name__ == "__main__":
    main()
