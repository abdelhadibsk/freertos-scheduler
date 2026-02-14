import re
import matplotlib.pyplot as plt

# =========================
# CONFIGURATION
# =========================
LOG_FILE = "traces/execution.log"
OUTPUT_PNG = "traces/schedule.png"

IGNORE_TASKS = {"Init", "IDLE"}

# =========================
# REGEX PATTERNS
# ========================= 
JOB_START_RE = re.compile(r"\[IN \] (\w+) at (\d+)")
JOB_END_RE   = re.compile(r"\[OUT\] (\w+) at (\d+)")
TASK_INFO_RE = re.compile(
    r"Task (\w+): period=(\d+), deadline=(\d+), exec_time=(\d+)"
)

# =========================
# DATA STRUCTURES
# =========================
start_times = {}
intervals = {}
task_periods = {}

# =========================
# PARSE LOG FILE
# =========================
with open(LOG_FILE, "r") as f:
    for line in f:

        # -------- Extract period from header --------
        task_info = TASK_INFO_RE.search(line)
        if task_info:
            name = task_info.group(1)
            period = int(task_info.group(2))
            task_periods[name] = period
            continue

        # -------- START --------
        start_match = JOB_START_RE.search(line)
        if start_match:
            task = start_match.group(1)
            time = int(start_match.group(2))

            if task in IGNORE_TASKS:
                continue

            start_times[task] = time

        # -------- END --------
        end_match = JOB_END_RE.search(line)
        if end_match:
            task = end_match.group(1)
            end_time = int(end_match.group(2))

            if task in IGNORE_TASKS:
                continue

            if task not in start_times:
                continue

            start_time = start_times[task]
            duration = end_time - start_time

            intervals.setdefault(task, []).append(
                (start_time, duration)
            )

# =========================
# FIND MAX TIME
# =========================
max_time = 0
for task in intervals:
    for start, dur in intervals[task]:
        max_time = max(max_time, start + dur)

# =========================
# ASCII TIMELINE
# =========================
print("\n================ ASCII SCHEDULING TIMELINE ================\n")

for task in sorted(intervals.keys()):
    for i, (start, dur) in enumerate(intervals[task], 1):
        print(f"{task}{i:<2} : [{start:>5} ---- {start + dur:>5}]")

# =========================
# GANTT DIAGRAM
# =========================
fig, ax = plt.subplots(figsize=(14, 5))

y = 0
yticks = []
ylabels = []

for task in sorted(intervals.keys()):

    # Draw execution bars
    ax.broken_barh(intervals[task], (y, 0.8))

    # 🔴 Draw thin red activation impulses
    if task in task_periods:
        period = task_periods[task]
        activation_times = range(0, max_time + period, period)

        for t in activation_times:
            ax.vlines(
                t,
                y + 0.15,   # bottom of spike
                y + 0.65,   # top of spike
                colors='red',
                linewidth=0.8
            )

    yticks.append(y + 0.4)
    ylabels.append(task)
    y += 1

ax.set_xlabel("Time")
ax.set_ylabel("Task")
ax.set_yticks(yticks)
ax.set_yticklabels(ylabels)
ax.set_title("Scheduling Timeline (RM Scheduler + Activations)")
ax.grid(True)

plt.tight_layout()
plt.savefig(OUTPUT_PNG)
plt.show()

print("\nGenerated file:")
print(f" - {OUTPUT_PNG}")
