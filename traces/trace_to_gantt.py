import re
import math
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from collections import defaultdict

# =========================
# CONFIGURATION
# =========================
LOG_FILE     = "traces/execution.log"
OUTPUT_PNG   = "traces/schedule.png"
IGNORE_TASKS = {"Init", "IDLE"}

# =========================
# REGEX PATTERNS
# =========================
TASK_INFO_RE  = re.compile(r"Task (\w+):\s+period=(\d+)\s+deadline=(\d+)\s+execution_time=(\d+)")
START_EXEC_RE = re.compile(r"\[START\]\s+(\w+)\s+tick=(\d+)")
END_EXEC_RE   = re.compile(r"\[END\s*\]\s+(\w+)\s+tick=(\d+)")
OUT_RE        = re.compile(r"\[OUT\]\s+(\w+)\s+at\s+(\d+)")
IN_RE         = re.compile(r"\[IN\s*\]\s+(\w+)\s+at\s+(\d+)")
POLICY_RE     = re.compile(r"Policy:\s*(.+)")

# =========================
# DATA STRUCTURES
# =========================
task_periods      = {}
task_deadlines    = {}
task_execution_time = {}
intervals         = {}
exec_start_times  = {}
preempted_tasks   = set()
time_offset       = None
scheduling_policy = "Unknown Policy"

# =========================
# PARSE LOG FILE
# =========================
with open(LOG_FILE, "r") as f:
    for line in f:

        task_info = TASK_INFO_RE.search(line)
        if task_info:
            name = task_info.group(1)
            task_periods[name]   = int(task_info.group(2))
            task_deadlines[name] = int(task_info.group(3))
            task_execution_time[name]      = int(task_info.group(4))
            continue

        policy_match = POLICY_RE.search(line)
        if policy_match:
            scheduling_policy = policy_match.group(1).strip()
            continue

        start_match = START_EXEC_RE.search(line)
        if start_match:
            task = start_match.group(1)
            tick = int(start_match.group(2))
            if time_offset is None:
                time_offset = tick
            exec_start_times[task] = tick - time_offset
            continue

        end_match = END_EXEC_RE.search(line)
        if end_match:
            task = end_match.group(1)
            tick = int(end_match.group(2))
            if task in IGNORE_TASKS:
                continue
            if task in exec_start_times:
                start_time = exec_start_times[task]
                end_time   = tick - time_offset
                duration   = end_time - start_time
                if duration > 0:
                    intervals.setdefault(task, []).append((start_time, duration))
                del exec_start_times[task]
            preempted_tasks.discard(task)
            continue

        out_match = OUT_RE.search(line)
        if out_match:
            task = out_match.group(1)
            tick = int(out_match.group(2))
            if task in IGNORE_TASKS:
                continue
            if task in exec_start_times:
                start_time = exec_start_times[task]
                end_time   = tick - time_offset
                duration   = end_time - start_time
                if duration > 0:
                    intervals.setdefault(task, []).append((start_time, duration))
                del exec_start_times[task]
                preempted_tasks.add(task)
            continue

        in_match = IN_RE.search(line)
        if in_match:
            task = in_match.group(1)
            tick = int(in_match.group(2))
            if task in IGNORE_TASKS:
                continue
            if task in preempted_tasks and time_offset is not None:
                exec_start_times[task] = tick - time_offset
            continue

# =========================
# SORT + MAX TIME
# =========================
for task in intervals:
    intervals[task].sort()

max_time = 0
for task in intervals:
    for start, dur in intervals[task]:
        max_time = max(max_time, start + dur)

n = len(intervals)

# =========================
# JOB INDEX HELPER
# =========================
def get_job_index(task, slice_start):
    period = task_periods.get(task)
    if not period:
        return 1
    return int(slice_start) // int(period) + 1

# =========================
# HEADER
# =========================
print("\n╔══════════════════════════════════════════════════════╗")
print("║           SCHEDULING TRACE ANALYZER                  ║")
print("╚══════════════════════════════════════════════════════╝")
print(f"\n  Policy: {scheduling_policy}\n")

# =========================
# TASK PARAMETERS
# =========================
print("─── Task Parameters ───────────────────────────────────")
print(f"{'Task':<8} {'Period':>8} {'Deadline':>10} {'Execution Time':>12}  {'Ui=Ci/Ti':>10}")
print("─" * 52)
total_util = 0.0
for task in sorted(intervals.keys(), key=lambda t: task_periods.get(t, 9999)):
    p  = task_periods.get(task, 0)
    d  = task_deadlines.get(task, 0)
    w  = task_execution_time.get(task, 0)
    ui = w / p if p > 0 else 0
    total_util += ui
    print(f"{task:<8} {p:>8} {d:>10} {w:>6}  {ui:>10.4f}")
print("─" * 52)
print(f"{'Total U:':<30} {total_util:.4f}  ({total_util*100:.2f}%)")

# =========================
# SCHEDULABILITY TEST
# =========================
policy_upper = scheduling_policy.upper()

print()
if n == 0:
    schedulable_label = "[!!] NO TASKS"
    schedulable       = "NO TASKS"

elif "EDF" in policy_upper:
    # Necessary AND sufficient for implicit deadlines (Liu & Layland 1973)
    ok = total_util <= 1.0
    print("─── Schedulability Test: EDF ──────────────────────────")
    print("  Theorem: EDF is optimal.")
    print("  Schedulable iff  U = sum(Ci/Ti) <= 1")
    print("  (necessary and sufficient for implicit deadlines)")
    print(f"\n  U = {total_util:.4f}  {'<= 1.0  =>' if ok else '>  1.0   =>'} {'SCHEDULABLE' if ok else 'DEADLINE MISS POSSIBLE'}")
    schedulable_label = "[OK]  SCHEDULABLE"          if ok else "[!!] DEADLINE MISS POSSIBLE"
    schedulable       = "SCHEDULABLE"                if ok else "DEADLINE MISS POSSIBLE"

elif "DM" in policy_upper:
    # Necessary:  U <= 1
    # Sufficient: Hyperbolic bound  PROD(Ci/Di + 1) <= 2  (Bini et al. 2003)
    hyp = 1.0
    for task in intervals:
        d = task_deadlines.get(task, task_periods.get(task, 1))
        w = task_execution_time.get(task, 0)
        hyp *= (w / d + 1)
    necessary  = total_util <= 1.0
    sufficient = hyp <= 2.0

    print("─── Schedulability Test: DM ───────────────────────────")
    print("  (1) Necessary:   U = sum(Ci/Ti) <= 1")
    print(f"      U = {total_util:.4f}  =>  {'PASSED' if necessary else 'FAILED -> definitely NOT schedulable'}")
    print()
    print("  (2) Sufficient:  Hyperbolic bound (Bini 2003)")
    print("      PROD(Ci/Di + 1) <= 2   [uses deadline Di]")
    print(f"      PROD = {hyp:.4f}  =>  {'PASSED -> guaranteed schedulable' if sufficient else 'INCONCLUSIVE -> use exact RTA'}")

    if not necessary:
        schedulable       = "NOT SCHEDULABLE (overloaded)"
        schedulable_label = "[!!] NOT SCHEDULABLE"
    elif sufficient:
        schedulable       = "SCHEDULABLE (sufficient cond.)"
        schedulable_label = "[OK]  SCHEDULABLE"
    else:
        schedulable       = "INCONCLUSIVE (use exact RTA)"
        schedulable_label = "[??] INCONCLUSIVE"
    print(f"\n  Overall: {schedulable}")

elif "RM" in policy_upper:
    # Sufficient (1): Liu & Layland  U <= n*(2^(1/n) - 1)
    # Sufficient (2): Hyperbolic     PROD(Ci/Ti + 1) <= 2  (Bini 2003, tighter)
    ll_bound = n * (2 ** (1 / n) - 1)
    ll_ok    = total_util <= ll_bound

    hyp = 1.0
    for task in intervals:
        p = task_periods.get(task, 1)
        w = task_execution_time.get(task, 0)
        hyp *= (w / p + 1)
    hyp_ok = hyp <= 2.0

    print("─── Schedulability Test: RM ───────────────────────────")
    print("  (1) Liu & Layland bound (sufficient):")
    print(f"      U <= n*(2^(1/n)-1)   n={n}  =>  bound = {ll_bound:.4f}")
    print(f"      U = {total_util:.4f}  =>  {'PASSED -> guaranteed schedulable' if ll_ok else 'FAILED (inconclusive)'}")
    print()
    print("  (2) Hyperbolic bound (Bini 2003, sufficient, tighter):")
    print("      PROD(Ci/Ti + 1) <= 2")
    print(f"      PROD = {hyp:.4f}  =>  {'PASSED -> guaranteed schedulable' if hyp_ok else 'INCONCLUSIVE -> use exact RTA'}")

    if ll_ok or hyp_ok:
        schedulable       = "SCHEDULABLE (sufficient cond.)"
        schedulable_label = "[OK]  SCHEDULABLE"
    else:
        schedulable       = "INCONCLUSIVE (use exact RTA)"
        schedulable_label = "[??] INCONCLUSIVE"
    print(f"\n  Overall: {schedulable}")

elif "FIFO" in policy_upper:
    ok = total_util < 1.0
    print("─── Schedulability Test: FIFO ─────────────────────────")
    print("  FIFO is not a real-time policy — no deadline guarantee.")
    print(f"  U = {total_util:.4f}  =>  {'No overload' if ok else 'OVERLOADED (U >= 1)'}")
    schedulable       = "NO RT GUARANTEE"
    schedulable_label = "[--] NO RT GUARANTEE"

else:
    ok = total_util <= 1.0
    print("─── Schedulability Test: Unknown policy ───────────────")
    print("  Applying necessary condition only: U <= 1")
    print(f"  U = {total_util:.4f}  =>  {'PASSED' if ok else 'FAILED (overloaded)'}")
    schedulable       = "UNKNOWN POLICY"
    schedulable_label = "[??] UNKNOWN POLICY"

# =========================
# EXECUTION SLICES
# =========================
print("\n─── Execution Slices per Task ─────────────────────────")
total_exec = defaultdict(int)
job_counts = defaultdict(int)
for task in sorted(intervals.keys(), key=lambda t: task_periods.get(t, 9999)):
    print(f"\n  {task}:")
    for i, (start, dur) in enumerate(intervals[task], 1):
        j = get_job_index(task, start)
        print(f"    J{j} slice {i:<3}: [{start:>6} ──── {start+dur:>6}]  (dur={dur})")
        total_exec[task] += dur
    w = task_execution_time.get(task, 1)
    job_counts[task] = round(total_exec[task] / w) if w > 0 else 0
    print(f"    => Total exec: {total_exec[task]}  |  Jobs completed: {job_counts[task]}")

# =========================
# RESPONSE TIMES
# =========================
print("\n─── Response Times ────────────────────────────────────")
print(f"{'Task':<8} {'Avg Resp':>10} {'Max Resp':>10} {'Execution Time':>12} {'Deadline':>10}  Status")
print("─" * 60)
for task in sorted(intervals.keys(), key=lambda t: task_periods.get(t, 9999)):
    d = task_deadlines.get(task, 0)
    w = task_execution_time.get(task, 0)
    slices = intervals[task]
    jobs = []
    acc = 0
    job_start = None
    for start, dur in slices:
        if job_start is None:
            job_start = start
        acc += dur
        if acc >= w:
            jobs.append(start + dur - job_start)
            acc = 0
            job_start = None
    if jobs:
        avg_r = sum(jobs) / len(jobs)
        max_r = max(jobs)
        miss  = "MISS !" if max_r > d else "OK"
        print(f"{task:<8} {avg_r:>10.1f} {max_r:>10}  {w:>6} {d:>10}  {miss}")

# =========================
# ASCII GANTT
# =========================
print("\n╔══════════════════════════════════════════════════════╗")
print("║                ASCII GANTT CHART                     ║")
print("╚══════════════════════════════════════════════════════╝\n")
for task in sorted(intervals.keys(), key=lambda t: task_periods.get(t, 9999)):
    for start, dur in intervals[task]:
        j = get_job_index(task, start)
        print(f"  {task} J{j:<2}: [{start:>6} ──── {start+dur:>6}]")

# =========================
# GANTT DIAGRAM
# =========================
TASK_COLORS = {
    task: color
    for task, color in zip(
        sorted(intervals.keys(), key=lambda t: task_periods.get(t, 9999)),
        ['#2196F3', '#4CAF50', '#FF5722', '#9C27B0', '#FF9800', '#00BCD4']
    )
}

fig, ax = plt.subplots(figsize=(18, max(4, n * 1.2 + 2)))
fig.patch.set_facecolor('#1a1a2e')
ax.set_facecolor('#16213e')

y              = 0
yticks         = []
ylabels        = []
legend_patches = []
sorted_tasks   = sorted(intervals.keys(), key=lambda t: task_periods.get(t, 9999))

for task in sorted_tasks:
    color = TASK_COLORS[task]

    ax.broken_barh(intervals[task], (y + 0.05, 0.75),
                   facecolors=color, edgecolors='white',
                   linewidth=0.4, alpha=0.88)

    for start, dur in intervals[task]:
        j  = get_job_index(task, start)
        cx = start + dur / 2
        if dur > 5:
            ax.text(cx, y + 0.42, f"J{j}",
                    ha='center', va='center',
                    fontsize=6.5, color='white', fontweight='bold')

    if task in task_periods:
        period   = task_periods[task]
        deadline = task_deadlines.get(task, period)
        for t in range(0, max_time + period, period):
            ax.annotate('', xy=(t, y + 0.05), xytext=(t, y - 0.25),
                        arrowprops=dict(arrowstyle='->', color='#00E676', lw=1.2))
            dl = t + deadline
            if dl <= max_time + period:
                ax.annotate('', xy=(dl, y + 0.8), xytext=(dl, y + 1.05),
                            arrowprops=dict(arrowstyle='->', color='#FF1744', lw=1.0))

    legend_patches.append(
        mpatches.Patch(facecolor=color, edgecolor='white', linewidth=0.5, label=task))
    yticks.append(y + 0.42)
    ylabels.append(task)
    y += 1.4

min_period = min(task_periods.values()) if task_periods else 100
xticks = list(range(0, max_time + min_period, min_period))
ax.set_xticks(xticks)
ax.set_xticklabels([str(t) for t in xticks], fontsize=7, color='#b0bec5', rotation=45)

for t in xticks:
    ax.axvline(t, color='#263254', linewidth=0.5, zorder=0)
for yi in range(len(sorted_tasks) + 1):
    ax.axhline(yi * 1.4 - 0.1, color='#263254', linewidth=0.4, zorder=0)

ax.set_xlim(left=0, right=max_time + 10)
ax.set_ylim(-0.4, y + 0.2)
ax.set_xlabel("Time  (ticks)", color='#b0bec5', fontsize=10, labelpad=8)
ax.set_ylabel("Task",          color='#b0bec5', fontsize=10, labelpad=8)
ax.set_yticks(yticks)
ax.set_yticklabels(ylabels, color='#eceff1', fontsize=10, fontweight='bold')
ax.tick_params(axis='x', colors='#b0bec5')
ax.tick_params(axis='y', colors='#eceff1')
for spine in ax.spines.values():
    spine.set_edgecolor('#263254')

info_patch  = mpatches.Patch(color='none', label=f"CPU Util: {total_util*100:.1f}%")
sched_patch = mpatches.Patch(color='none', label=schedulable_label)
ax.legend(handles=legend_patches + [info_patch, sched_patch],
          loc='upper right', framealpha=0.25,
          facecolor='#0f3460', edgecolor='#263254',
          labelcolor='white', fontsize=8)

ax.annotate('▲ activation', xy=(0.01, 0.02), xycoords='axes fraction',
            fontsize=7, color='#00E676')
ax.annotate('▼ deadline',   xy=(0.08, 0.02), xycoords='axes fraction',
            fontsize=7, color='#FF1744')

ax.set_title(f"{scheduling_policy}  —  Gantt Chart",
             color='#eceff1', fontsize=13, fontweight='bold', pad=14)

annot = ax.annotate(
    "", xy=(0, 0), xytext=(10, 10),
    textcoords="offset points",
    bbox=dict(boxstyle="round,pad=0.3", fc="#0f3460", ec="#FFD600", lw=0.8),
    color="white", fontsize=8, visible=False
)

def on_hover(event):
    if event.inaxes != ax:
        annot.set_visible(False)
        fig.canvas.draw_idle()
        return
    x = event.xdata
    if x is None:
        return
    found = False
    for task_y, task in zip(yticks, sorted_tasks):
        if (task_y - 0.37) <= event.ydata <= (task_y + 0.37):
            for start, dur in intervals[task]:
                if start <= x <= start + dur:
                    j = get_job_index(task, start)
                    annot.xy = (x, event.ydata)
                    annot.set_text(
                        f"{task}  J{j}\n"
                        f"start={start}  end={start+dur}\n"
                        f"dur={dur}  execution_time={task_execution_time.get(task,'?')}"
                    )
                    annot.set_visible(True)
                    fig.canvas.draw_idle()
                    found = True
                    break
    if not found:
        annot.set_visible(False)
        fig.canvas.draw_idle()

fig.canvas.mpl_connect("motion_notify_event", on_hover)

plt.tight_layout()
plt.savefig(OUTPUT_PNG, dpi=150, bbox_inches='tight', facecolor=fig.get_facecolor())
plt.show()

print(f"\nGantt saved -> {OUTPUT_PNG}")