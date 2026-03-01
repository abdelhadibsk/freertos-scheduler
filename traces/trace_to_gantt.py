import re
import matplotlib.pyplot as plt
import itertools

# =========================
# CONFIGURATION
# =========================
LOG_FILE = "traces/execution.log"
OUTPUT_PNG = "traces/schedule.png"

IGNORE_TASKS = {"Init", "IDLE"}

# =========================
# REGEX PATTERNS
# =========================
TASK_INFO_RE = re.compile(
    r"Task (\w+): period=(\d+)\s+deadline=(\d+)\s+wcet=(\d+)"
)

START_EXEC_RE = re.compile(r"\[START\]\s+(\w+)\s+tick=(\d+)")
END_EXEC_RE   = re.compile(r"\[END\s*\]\s+(\w+)\s+tick=(\d+)")
OUT_RE        = re.compile(r"\[OUT\]\s+(\w+)\s+at\s+(\d+)")

# =========================
# DATA STRUCTURES
# =========================
task_periods = {}
intervals = {}
exec_start_times = {}
time_offset = None

# =========================
# PARSE LOG FILE
# =========================
with open(LOG_FILE, "r") as f:
    for line in f:

        # Extract task info (period)
        task_info = TASK_INFO_RE.search(line)
        if task_info:
            name = task_info.group(1)
            period = int(task_info.group(2))
            task_periods[name] = period
            continue

        # Execution START
        start_match = START_EXEC_RE.search(line)
        if start_match:
            task = start_match.group(1)
            tick = int(start_match.group(2))

            # define time offset from first execution
            if time_offset is None:
                time_offset = tick

            exec_start_times[task] = tick - time_offset
            continue

        # Execution END (normal finish)
        end_match = END_EXEC_RE.search(line)
        if end_match:
            task = end_match.group(1)
            tick = int(end_match.group(2))

            if task in IGNORE_TASKS:
                continue

            if task in exec_start_times:
                start_time = exec_start_times[task]
                end_time = tick - time_offset
                duration = end_time - start_time

                intervals.setdefault(task, []).append(
                    (start_time, duration)
                )

                del exec_start_times[task]
            continue

        # Context switch OUT (possible preemption)
        out_match = OUT_RE.search(line)
        if out_match:
            task = out_match.group(1)
            tick = int(out_match.group(2))

            if task in IGNORE_TASKS:
                continue

            if task in exec_start_times:
                start_time = exec_start_times[task]
                end_time = tick - time_offset
                duration = end_time - start_time

                intervals.setdefault(task, []).append(
                    (start_time, duration)
                )

                del exec_start_times[task]
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
# AUTO TICK STEP SELECTION
# =========================
def choose_tick_step(max_time):
    if max_time <= 200:
        return 10
    elif max_time <= 1000:
        return 50
    elif max_time <= 5000:
        return 100
    else:
        return 500

tick_step = choose_tick_step(max_time)
minor_step = tick_step // 5

# =========================
# ASCII TIMELINE
# =========================
print("\n================ ASCII SCHEDULING TIMELINE ================\n")

for task in sorted(intervals.keys(), key=lambda t: task_periods.get(t, 9999)):
    for i, (start, dur) in enumerate(intervals[task], 1):
        print(f"{task}{i:<2} : [{start:>5} ---- {start + dur:>5}]")

# =========================
# GANTT DIAGRAM
# =========================
fig, ax = plt.subplots(figsize=(16, 6))

y = 0
yticks = []
ylabels = []

colors = itertools.cycle(plt.cm.tab10.colors)

for task in sorted(intervals.keys(), key=lambda t: task_periods.get(t, 9999)):

    color = next(colors)

    ax.broken_barh(
        intervals[task],
        (y, 0.8),
        facecolors=color
    )

    # Activation impulses (RM)
    if task in task_periods:
        period = task_periods[task]
        activation_times = range(0, max_time + period, period)

        for t in activation_times:
            ax.vlines(
                t,
                y + 0.15,
                y + 0.65,
                colors='red',
                linewidth=0.7
            )

    yticks.append(y + 0.4)
    ylabels.append(task)
    y += 1

# =========================
# AXIS FORMATTING
# =========================

ax.set_xlim(0, max_time)
ax.set_xlabel("Time (ticks)")
ax.set_ylabel("Task")
ax.set_yticks(yticks)
ax.set_yticklabels(ylabels)

# Major ticks
ax.set_xticks(range(0, max_time + tick_step, tick_step))

# Minor ticks
ax.set_xticks(range(0, max_time + minor_step, minor_step), minor=True)

# Grid styling
ax.grid(which='major', axis='x', linestyle='-', linewidth=0.8)
ax.grid(which='minor', axis='x', linestyle='--', linewidth=0.4, alpha=0.5)
ax.grid(which='major', axis='y', linestyle='--', alpha=0.4)

ax.set_title("Scheduling Timeline (Rate Monotonic)")

plt.tight_layout()
plt.savefig(OUTPUT_PNG, dpi=300)
plt.show()