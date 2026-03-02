import re
import itertools
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from collections import defaultdict

# =========================
# CONFIGURATION
# =========================
LOG_FILE    = "traces/execution.log"
OUTPUT_PNG  = "traces/schedule.png"
IGNORE_TASKS = {"Init", "IDLE"}

# =========================
# REGEX PATTERNS
# =========================
TASK_INFO_RE  = re.compile(r"Task (\w+):\s+period=(\d+)\s+deadline=(\d+)\s+wcet=(\d+)")
START_EXEC_RE = re.compile(r"\[START\]\s+(\w+)\s+tick=(\d+)")
END_EXEC_RE   = re.compile(r"\[END\s*\]\s+(\w+)\s+tick=(\d+)")
OUT_RE        = re.compile(r"\[OUT\]\s+(\w+)\s+at\s+(\d+)")
IN_RE         = re.compile(r"\[IN\s*\]\s+(\w+)\s+at\s+(\d+)")
POLICY_RE     = re.compile(r"Policy:\s*(.+)")

# =========================
# DATA STRUCTURES
# =========================
task_periods     = {}   # task -> period
task_deadlines   = {}   # task -> deadline
task_wcet        = {}   # task -> wcet
intervals        = {}   # task -> [(start, duration), ...]
exec_start_times = {}   # task -> current slice start tick
preempted_tasks  = set()  # tasks that were preempted and not yet finished
time_offset      = None
scheduling_policy = "Unknown Policy"

# =========================
# PARSE LOG FILE
# =========================
with open(LOG_FILE, "r") as f:
    for line in f:

        # Task info (period / deadline / wcet)
        task_info = TASK_INFO_RE.search(line)
        if task_info:
            name = task_info.group(1)
            task_periods[name]   = int(task_info.group(2))
            task_deadlines[name] = int(task_info.group(3))
            task_wcet[name]      = int(task_info.group(4))
            continue

        # Scheduling policy
        policy_match = POLICY_RE.search(line)
        if policy_match:
            scheduling_policy = policy_match.group(1).strip()
            continue

        # [START] -> open execution slice
        start_match = START_EXEC_RE.search(line)
        if start_match:
            task = start_match.group(1)
            tick = int(start_match.group(2))
            if time_offset is None:
                time_offset = tick
            exec_start_times[task] = tick - time_offset
            continue

        # [END] -> close slice (normal finish)
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
            # Job fully done — remove from preempted set
            preempted_tasks.discard(task)
            continue

        # [OUT] -> close slice (preemption or finish)
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
                # Mark as preempted — it has NOT finished its job yet
                preempted_tasks.add(task)
            continue

        # [IN] -> task resumed after preemption: reopen its slice
        in_match = IN_RE.search(line)
        if in_match:
            task = in_match.group(1)
            tick = int(in_match.group(2))
            if task in IGNORE_TASKS:
                continue
            if task in preempted_tasks and time_offset is not None:
                exec_start_times[task] = tick - time_offset
                # Don't remove from preempted_tasks yet — [END] will clear it
            continue

# =========================
# SORT INTERVALS
# =========================
for task in intervals:
    intervals[task].sort()

# =========================
# FIND MAX TIME
# =========================
max_time = 0
for task in intervals:
    for start, dur in intervals[task]:
        max_time = max(max_time, start + dur)

# =========================
# JOB INDEX HELPER
# A slice belongs to job k if (k-1)*period <= slice_start < k*period
# This correctly groups preempted fragments under the same job number.
# =========================
def get_job_index(task, slice_start):
    period = task_periods.get(task)
    if not period:
        return 1
    return int(slice_start) // int(period) + 1

# =========================
# CPU UTILIZATION STATS
# =========================
print("\n╔══════════════════════════════════════════════════════╗")
print("║           SCHEDULING TRACE ANALYZER                  ║")
print("╚══════════════════════════════════════════════════════╝\n")

print("─── Task Parameters ───────────────────────────────────")
print(f"{'Task':<8} {'Period':>8} {'Deadline':>10} {'WCET':>6}  {'U_i':>8}")
print("─" * 50)
total_util = 0.0
for task in sorted(intervals.keys(), key=lambda t: task_periods.get(t, 9999)):
    p = task_periods.get(task, 0)
    d = task_deadlines.get(task, 0)
    w = task_wcet.get(task, 0)
    ui = w / p if p > 0 else 0
    total_util += ui
    print(f"{task:<8} {p:>8} {d:>10} {w:>6}  {ui:>7.3f}")
print("─" * 50)
print(f"{'Total CPU Utilization:':<30} {total_util:.3f} ({total_util*100:.1f}%)")

# RM schedulability bound: n*(2^(1/n)-1)
import math
n = len(intervals)
if n > 0:
    rm_bound = n * (2 ** (1/n) - 1)
    schedulable_label = "[OK]  SCHEDULABLE" if total_util <= rm_bound else "[!!] NOT GUARANTEED"
    schedulable = "SCHEDULABLE" if total_util <= rm_bound else "NOT GUARANTEED"
    print(f"{'RM Bound (Liu & Layland):':<30} {rm_bound:.3f}")
    print(f"{'RM Schedulability:':<30} {'✅ ' + schedulable if total_util <= rm_bound else '⚠️  ' + schedulable}")
else:
    rm_bound = 1.0
    schedulable_label = "[!!] NO TASKS"
    schedulable = "NO TASKS"

print("\n─── Execution Slices per Task ─────────────────────────")
total_exec = defaultdict(int)
job_counts  = defaultdict(int)
for task in sorted(intervals.keys(), key=lambda t: task_periods.get(t, 9999)):
    print(f"\n  {task}:")
    for i, (start, dur) in enumerate(intervals[task], 1):
        print(f"    slice {i:<3}: [{start:>6} ──── {start+dur:>6}]  (dur={dur})")
        total_exec[task] += dur
    # count jobs: total_exec / wcet
    w = task_wcet.get(task, 1)
    job_counts[task] = round(total_exec[task] / w) if w > 0 else 0
    print(f"    → Total exec: {total_exec[task]}  |  Jobs completed: {job_counts[task]}")

print("\n─── Response Times ────────────────────────────────────")
print(f"{'Task':<8} {'Avg Resp':>10} {'Max Resp':>10} {'WCET':>6} {'Deadline':>10}")
print("─" * 50)
for task in sorted(intervals.keys(), key=lambda t: task_periods.get(t, 9999)):
    p = task_periods.get(task, 0)
    d = task_deadlines.get(task, 0)
    w = task_wcet.get(task, 0)
    # Group slices into jobs (each job = consecutive slices summing to wcet)
    slices = intervals[task]
    jobs = []
    acc = 0
    job_start = None
    for start, dur in slices:
        if job_start is None:
            job_start = start
        acc += dur
        if acc >= w:
            jobs.append(start + dur - job_start)  # response time = finish - release
            acc = 0
            job_start = None
    if jobs:
        avg_r = sum(jobs) / len(jobs)
        max_r = max(jobs)
        miss = "❌ MISS" if max_r > d else "✅ OK"
        print(f"{task:<8} {avg_r:>10.1f} {max_r:>10}  {w:>6} {d:>10}  {miss}")

# =========================
# ASCII GANTT
# =========================
print("\n\n╔══════════════════════════════════════════════════════╗")
print("║                ASCII GANTT CHART                     ║")
print("╚══════════════════════════════════════════════════════╝\n")
for task in sorted(intervals.keys(), key=lambda t: task_periods.get(t, 9999)):
    for start, dur in intervals[task]:
        j = get_job_index(task, start)
        print(f"  {task}J{j:<2} : [{start:>6} ──── {start+dur:>6}]")

# =========================
# GANTT DIAGRAM (matplotlib)
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

y = 0
yticks  = []
ylabels = []
legend_patches = []

sorted_tasks = sorted(intervals.keys(), key=lambda t: task_periods.get(t, 9999))

for task in sorted_tasks:
    color = TASK_COLORS[task]

    # Draw execution bars
    ax.broken_barh(
        intervals[task],
        (y + 0.05, 0.75),
        facecolors=color,
        edgecolors='white',
        linewidth=0.4,
        alpha=0.88
    )

    # Label each slice with its job index (based on activation period)
    for start, dur in intervals[task]:
        j = get_job_index(task, start)
        cx = start + dur / 2
        if dur > 5:  # only label if wide enough
            ax.text(
                cx, y + 0.42,
                f"J{j}",
                ha='center', va='center',
                fontsize=6.5, color='white',
                fontweight='bold'
            )

    # Activation / deadline impulses
    if task in task_periods:
        period   = task_periods[task]
        deadline = task_deadlines.get(task, period)
        for t in range(0, max_time + period, period):
            # Activation arrow (green upward)
            ax.annotate(
                '', xy=(t, y + 0.05), xytext=(t, y - 0.25),
                arrowprops=dict(arrowstyle='->', color='#00E676', lw=1.2)
            )
            # Deadline marker (red downward)
            dl = t + deadline
            if dl <= max_time + period:
                ax.annotate(
                    '', xy=(dl, y + 0.8), xytext=(dl, y + 1.05),
                    arrowprops=dict(arrowstyle='->', color='#FF1744', lw=1.0)
                )

    legend_patches.append(
        mpatches.Patch(facecolor=color, edgecolor='white', linewidth=0.5, label=task)
    )
    yticks.append(y + 0.42)
    ylabels.append(task)
    y += 1.4

# ── X axis ticks: every period of smallest task ──────────────────────────────
min_period = min(task_periods.values()) if task_periods else 100
tick_step  = min_period
xticks = list(range(0, max_time + tick_step, tick_step))
ax.set_xticks(xticks)
ax.set_xticklabels(
    [str(t) for t in xticks],
    fontsize=7, color='#b0bec5', rotation=45
)

# ── Vertical grid at every tick ──────────────────────────────────────────────
for t in xticks:
    ax.axvline(t, color='#263254', linewidth=0.5, zorder=0)

# ── Horizontal separators ─────────────────────────────────────────────────────
for yi in range(len(sorted_tasks) + 1):
    ax.axhline(yi * 1.4 - 0.1, color='#263254', linewidth=0.4, zorder=0)

# ── Styling ───────────────────────────────────────────────────────────────────
ax.set_xlim(left=0, right=max_time + 10)
ax.set_ylim(-0.4, y + 0.2)
ax.set_xlabel("Time  (ticks)", color='#b0bec5', fontsize=10, labelpad=8)
ax.set_ylabel("Task", color='#b0bec5', fontsize=10, labelpad=8)
ax.set_yticks(yticks)
ax.set_yticklabels(ylabels, color='#eceff1', fontsize=10, fontweight='bold')
ax.tick_params(axis='x', colors='#b0bec5')
ax.tick_params(axis='y', colors='#eceff1')

for spine in ax.spines.values():
    spine.set_edgecolor('#263254')

# ── Legend ────────────────────────────────────────────────────────────────────
util_text  = f"CPU Util: {total_util*100:.1f}%"
sched_text = f"RM: {schedulable_label}"
info_patch = mpatches.Patch(color='none', label=util_text)
sched_patch = mpatches.Patch(color='none', label=sched_text)

legend = ax.legend(
    handles=legend_patches + [info_patch, sched_patch],
    loc='upper right',
    framealpha=0.25,
    facecolor='#0f3460',
    edgecolor='#263254',
    labelcolor='white',
    fontsize=8
)

# ── Arrows legend note ────────────────────────────────────────────────────────
ax.annotate('▲ activation', xy=(0.01, 0.02), xycoords='axes fraction',
            fontsize=7, color='#00E676')
ax.annotate('▼ deadline',   xy=(0.08, 0.02), xycoords='axes fraction',
            fontsize=7, color='#FF1744')

# ── Title ─────────────────────────────────────────────────────────────────────
ax.set_title(
    f"{scheduling_policy}  —  Gantt Chart",
    color='#eceff1', fontsize=13, fontweight='bold', pad=14
)

# ── Interactive cursor (vertical crosshair) ───────────────────────────────────
# ── Hover tooltip ─────────────────────────────────────────────────────────────
annot = ax.annotate(
    "", xy=(0, 0), xytext=(10, 10),
    textcoords="offset points",
    bbox=dict(boxstyle="round,pad=0.3", fc="#0f3460", ec="#FFD600", lw=0.8),
    color="white", fontsize=8,
    visible=False
)

def on_hover(event):
    if event.inaxes != ax:
        annot.set_visible(False)
        fig.canvas.draw_idle()
        return
    x = event.xdata
    if x is None:
        return
    # find which task / slice we're over
    found = False
    for task_y, task in zip(yticks, sorted_tasks):
        y_bot = task_y - 0.37
        y_top = task_y + 0.37
        if y_bot <= event.ydata <= y_top:
            for start, dur in intervals[task]:
                if start <= x <= start + dur:
                    j = get_job_index(task, start)
                    annot.xy = (x, event.ydata)
                    annot.set_text(
                        f"{task}  J{j}\n"
                        f"start={start}  end={start+dur}\n"
                        f"dur={dur}  wcet={task_wcet.get(task,'?')}"
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

print(f"\n✅ Gantt saved → {OUTPUT_PNG}")