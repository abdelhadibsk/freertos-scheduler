import re
import matplotlib.pyplot as plt

# =========================
# CONFIGURATION
# =========================
LOG_FILE = "execution.log"
OUTPUT_PNG = "schedule.png"
OUTPUT_PDF = "schedule.pdf"

# =========================
# REGEX PATTERNS (MATCH YOUR LOG)
# =========================
JOB_START_RE = re.compile(r"\[JOB START\] Task (\w+) at (\d+)")
JOB_END_RE   = re.compile(r"\[JOB END\] Task (\w+) at (\d+)")

# =========================
# DATA STRUCTURES
# =========================
start_times = {}    # temporary: task -> start time
intervals = {}      # final: task -> list of (start, duration)

# =========================
# PARSE LOG FILE
# =========================
with open(LOG_FILE, "r") as f:
    for line in f:
        start_match = JOB_START_RE.search(line)
        end_match = JOB_END_RE.search(line)

        if start_match:
            task = start_match.group(1)
            time = int(start_match.group(2))
            start_times[task] = time

        elif end_match:
            task = end_match.group(1)
            end_time = int(end_match.group(2))

            if task not in start_times:
                continue  # safety check

            start_time = start_times[task]
            duration = end_time - start_time

            intervals.setdefault(task, []).append(
                (start_time, duration)
            )

# =========================
# ASCII TIMELINE (TERMINAL)
# =========================
print("\n================ ASCII SCHEDULING TIMELINE ================\n")

for task in sorted(intervals.keys()):
    for i, (start, dur) in enumerate(intervals[task], 1):
        print(f"{task}{i:<2} : [{start:>5} ---- {start + dur:>5}]")

# =========================
# GANTT DIAGRAM (PNG + PDF)
# =========================
fig, ax = plt.subplots(figsize=(12, 4))

y = 0
yticks = []
ylabels = []

for task in sorted(intervals.keys()):
    ax.broken_barh(intervals[task], (y, 0.8))
    yticks.append(y + 0.4)
    ylabels.append(task)
    y += 1

ax.set_xlabel("Time")
ax.set_ylabel("Task")
ax.set_yticks(yticks)
ax.set_yticklabels(ylabels)
ax.set_title("Scheduling Timeline (RM Scheduler)")
ax.grid(True)

plt.tight_layout()
plt.savefig(OUTPUT_PNG)
plt.savefig(OUTPUT_PDF)
plt.show()

print("\nGenerated files:")
print(f" - {OUTPUT_PNG}")
print(f" - {OUTPUT_PDF}")
