# Fills in page.template.html from the walk's real output.
import json, os, statistics, sys
out = sys.argv[1]
page = open("page.template.html").read()
lines = open("official.txt").read().splitlines()
sentence = lines[1]
journey = json.load(open("journey.json"))
final = journey["steps"][-1]
lies = final[0]
official = {"lies": lies, "claims": final[2:]}
example = {"opening": "Thanks for the pancakes: on a Saturday morning, this sentence told exactly", "lies": 105686891,
           "claims": [13,2,5,12,56,12,8,24,29,1,3,12,3,33,18,2,1,18,37,45,10,9,8,5,13,1]}
walks = [int(l.split("lies=")[1].split()[0]) for l in open("other-walks.txt") if l.startswith("TRUE")]
walks.sort()
median = walks[len(walks) // 2]
longer = sum(1 for w in walks if w < lies)
other = (f"I also ran {len(walks)} walks from other seeds, so other true sentences exist with other numbers. "
         f"The median walk took about {round(median / 1e6)} million lies, the shortest {walks[0]:,} and the longest about {walks[-1] / 1e9:.1f} billion. "
         f"This one ran longer than {round(100 * longer / len(walks))}% of them.")
print(other)
p90 = walks[int(len(walks) * .9)]
seconds_per_lie = 19 / lies
toy = f"On the mini, nine walks in ten would finish in about {max(10, round(p90 * seconds_per_lie / 10) * 10)} seconds or less."
print(toy)
recheck = open("recheck.txt").read() if os.path.exists("recheck.txt") else ""
if f"after {lies} false" in recheck and sentence in recheck:
    note = (f"Four separate counts agree. The C program found it, a Python script recounted the final letters, a second program wrote out all {lies + 1:,} "
            "sentences in full and counted each one from its text, and the JavaScript on this page stops at the same place.")
else:
    sys.exit("recheck has not confirmed the walk: " + recheck[:200])
frames = "[" + ",".join(json.dumps(f, separators=(",", ":")) for f in journey["steps"]) + "]"
for key, value in {
    "SENTENCE": sentence, "WALK_JS": open("walk.js").read(), "OFFICIAL": json.dumps(official, separators=(",", ":")),
    "EXAMPLE": json.dumps(example), "EXAMPLE_LIES": f"{example['lies']:,}", "FRAMES": frames,
    "RECHECK": note, "OTHER_WALKS": other, "TOY_TIME": toy,
}.items():
    page = page.replace("{{" + key + "}}", value)
open(out, "w").write(page)
print("wrote", out, len(page), "bytes")
