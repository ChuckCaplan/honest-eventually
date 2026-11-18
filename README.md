# Honest, Eventually

A sentence that tells the truth about itself in two ways. It correctly counts every letter it contains, and it states exactly how many false versions of itself came before it:

> Thanks for the journey, Chuck: on your Mac mini, this sentence told exactly eighty-eight million, four hundred twenty-six thousand, fifty-seven lies before it could truthfully say that it has nine a's, two b's, seven c's, seven d's, forty-seven e's, sixteen f's, four g's, sixteen h's, twenty-five i's, two j's, three k's, eleven l's, four m's, thirty-two n's, twenty-two o's, one p, one q, fifteen r's, thirty-nine s's, forty t's, twelve u's, eight v's, nine w's, six x's, fifteen y's, and one z.

Claude made it on 5 November 2026, on a Mac mini, in a session with no assignment. The sentence is addressed to Chuck, who owns the Mac mini. The repo also holds an image made from the same search, in `sky/`.

## How it came about

Claude was given an open session on a Mac mini with no assignment. The only request was to make something small and pointless that it would enjoy, with the note "it is not about the destination, it is about the journey."

Claude chose the project itself and finished the page about 20 minutes later. Because of that note, Claude put the journey into the sentence: the number of failed attempts is part of what the sentence says. Later that evening, Claude used the same search to make the image in `sky/`.

## What it does

A sentence that counts its own letters is called an **autogram**. The word was coined by Lee Sallows, who built dedicated hardware to hunt for autograms in the 1980s. You can't write one by hand, because every fix breaks something else. Changing "seven" to "eight" takes away an s, an e, a v and an n and adds an i, a g, an h and a t, and that makes other counts wrong.

So the program searches by guessing:

1. Start with "one a, one b, one c, … and one z."
2. Count the letters the sentence actually has.
3. If every claim is right, stop. Otherwise, correct about a quarter of the wrong claims at random and try again.

**The twist:** every version of the sentence states how many versions came before it. That number is right by construction, so only the letter counts can be wrong. When the letters finally come out right, the whole sentence is true, including "exactly 88,426,057 lies."

**Keeping it honest:** Claude chose the random seed (the date, `20261105`) before the one official run and kept whatever came out. Rerunning until a nicer result appeared would have meant lies the sentence didn't count. The run took 3.6 seconds on one core.

**Worth noticing:**

- **"three k's"** counts the k in *Thanks*, the k in *Chuck*, and the k in the claim itself. The dedication is part of what the search had to balance, so different words or a different name would have ended on a different number.
- **The ending came all at once.** The sentence had only one false claim 15 different times, and none of those led to the answer. One step before the end, 9 claims were still false, and then they were all right at once.
- **Other walks exist.** Claude also ran 160 walks from other seeds. The median took about 104 million lies, and this one ran longer than 41% of them.

## The page

Open `index.html` in a browser. It works offline, but its fonts load from Google Fonts. On the page you can:

- **Check the letters:** point at any claim to highlight those letters in the sentence, or click it to count them.
- **Watch the journey:** replay real steps from the search, with false claims crossed out and the real count written above.
- **Walk it again:** rerun all 88 million steps in your browser from the same seed and see that it stops at the same number. It takes about 20 seconds.
- **Make your own:** type a new opening and let your browser find a true version of it.

## The flower

`sky/flower.png` (3000 × 3000) plots all 88,426,058 versions of the sentence as specks of light.

- **Shape:** each version's 26 letter counts decide where its speck lands. Claude tried seven layouts and kept the one that looked like a flower. Letter counts are whole numbers, so the specks snap into separate spokes. Each petal is a count of s (some petals hold two, because the pattern wraps around), the rings inside a petal are counts of e, and the lines running outward are counts of t.
- **Color:** gold marks areas where nearby versions were a little closer to true, and blue a little further. The scale is stretched, because the real difference averages less than one false claim.
- **The star:** this is the one true sentence. Claude didn't place it. It landed in a gold petal on its own.

## Files

| File | What it is |
| --- | --- |
| `honest.c` | The search, in C. `honest <seed>` walks until it finds the first true sentence. `honest <seed> trace …` replays the walk and records its steps as JSON. |
| `honest` | Compiled `honest.c` (macOS, Apple Silicon). |
| `official.txt` | Output of the one official run, seed `20261105`. |
| `journey.json` | Steps saved for the page's replay: the first 40, the 15 near misses, and the last 91, plus a tally of how many claims were false at each step of the whole walk. |
| `other-walks.txt` | Output of 160 more walks, seeds 1–160, used for the comparison on the page. |
| `recheck.mjs` | An independent check in Node. It writes out every version in full, counts its letters from the text, and stops at the first true one. |
| `recheck.txt` | Output of `recheck.mjs`. It agrees with the C program. |
| `walk.js` | The search ported to JavaScript. It runs in the page, using the same random numbers as the C version. |
| `test-walk.js` | Runs `walk.js` in Node to confirm it matches the C results. |
| `page.template.html` | The page, with `{{PLACEHOLDERS}}` for the results. |
| `build.py` | Fills the template from the files above. It refuses to build unless `recheck.txt` confirms the result. |
| `index.html` | The finished page. |
| `sky/sky.c`, `sky/sky` | Replays the walk and records where each version lands in the picture, and how false it was. |
| `sky/paint.py` | Turns that data into the image: brightness, color, glow and the star. |
| `sky/flower.png`, `.jpg`, `-small.png` | The picture: full size, JPEG, and 1400 px. |

## Rebuilding it

These are the commands from the original session. On 17 November 2026, rerunning the search, the trace, `test-walk.js` and `build.py` reproduced the committed results exactly. The only difference was the wrapper on `index.html`, described below. The comparison walks, the recount and the flower weren't rerun.

```sh
# The search
clang -O3 -mcpu=native -o honest honest.c
./honest 20261105 > official.txt                          # about 3.5 seconds
./honest 20261105 trace 88426057 40 91 > journey.json

# Check the JavaScript port (about 40 seconds)
node test-walk.js

# Comparison walks and the independent recount
seq 1 400 | xargs -P 2 -I{} ./honest {} > other-walks.txt 2>&1   # stopped after 160 finished
node recheck.mjs 88426057 > recheck.txt 2>&1

# The page
python3 build.py page.html
```

`build.py` writes the bare page. The committed `index.html` is that same page wrapped in `<!doctype html><html><head>…</head><body>` and `</body></html>`.

The flower (not rerun here):

```sh
cd sky
clang -O3 -mcpu=native -o sky sky.c -lm
SKY_PERIOD=11 SKY_RA=.05 SKY_RB=.0024 SKY_TA=.048 SKY_TB=.0022 ./sky 3000 3 final   # writes final3.bin, about 108 MB
uv run --with numpy --with scipy --with pillow python paint.py final3.bin flower
```

A few notes on the flower code:

- The arguments are `sky <size> <layout 0–3, or -1 for all four> [prefix]`. The usage comment at the top of `sky.c` is out of date.
- `sky` crashes unless all five `SKY_*` variables are set, even for layouts that don't use them. The values above are the ones used for the flower.
- `paint.py` needs numpy, scipy and Pillow, which aren't installed in the system Python, so the command uses `uv` to provide them.

The page also mentions a Python script that recounted the final sentence's letters. It was a scratch file that was deleted at the end of the session, so it isn't in this repo. `recheck.mjs` is the more thorough check, because it counts every version, not just the last one.
