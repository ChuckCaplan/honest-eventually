const ONES = ["", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve",
  "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"];
const TENS = ["", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"];
const under100 = (n) => (n < 20 ? ONES[n] : n % 10 ? TENS[n / 10 | 0] + "-" + ONES[n % 10] : TENS[n / 10]);
const group = (n) => (n >= 100 ? ONES[n / 100 | 0] + " hundred" + (n % 100 ? " " + under100(n % 100) : "") : under100(n));

function spellCount(n) {
  if (n === 0) return "zero";
  const parts = [];
  for (const [size, name] of [[1e9, " billion"], [1e6, " million"], [1e3, " thousand"], [1, ""]]) {
    const part = Math.floor(n / size) % 1000;
    if (part) parts.push(group(part) + name);
  }
  return parts.join(", ");
}

function tally(text, into) {
  for (let j = 0; j < text.length; j++) {
    const code = text.charCodeAt(j) | 32;
    if (code >= 97 && code <= 122) into[code - 97]++;
  }
}

// One walk: start at "one a, one b, ..." and on each step correct about a quarter of the
// claims at random, until a sentence comes out true. Every sentence states the exact number
// of false sentences before it, so only its letters can be wrong.
function walk(opening, middle, seedHi, seedLo, onProgress) {
  const num = new Int16Array(100 * 26);
  const grp = new Int16Array(1000 * 26);
  const scratch = new Int32Array(26);
  for (let n = 1; n < 100; n++) {
    scratch.fill(0);
    tally(under100(n), scratch);
    if (n !== 1) scratch[18]++;
    num.set(scratch, n * 26);
  }
  for (let n = 1; n < 1000; n++) {
    scratch.fill(0);
    tally(group(n), scratch);
    grp.set(scratch, n * 26);
  }
  const fixed = new Int32Array(26);
  tally(opening, fixed);
  tally("lies", fixed);
  tally(middle, fixed);
  tally("and", fixed);
  for (let i = 0; i < 26; i++) fixed[i]++;

  const v = new Int32Array(26).fill(1);
  const claims = new Int32Array(26);
  for (let i = 0; i < 26; i++) for (let j = 0; j < 26; j++) claims[j] += num[26 + j];
  const slow = new Int32Array(26);
  const c = new Int32Array(26);

  // splitmix64 on two 32-bit halves, so that a seed gives the same walk as the C program.
  let hi = seedHi >>> 0, lo = seedLo >>> 0, zh = 0, zl = 0;
  const mul = (chi, clo) => {
    const a0 = zl & 0xffff, a1 = zl >>> 16, b0 = clo & 0xffff, b1 = clo >>> 16;
    const t1 = a0 * b0;
    const t2 = a1 * b0 + (t1 >>> 16);
    const t3 = a0 * b1 + (t2 & 0xffff);
    const high = a1 * b1 + (t2 >>> 16) + (t3 >>> 16);
    const nl = (((t3 & 0xffff) << 16) | (t1 & 0xffff)) >>> 0;
    zh = (high + Math.imul(zh, clo) + Math.imul(zl, chi)) >>> 0;
    zl = nl;
  };

  for (let t = 0; ; t++) {
    const low = t % 1000;
    if (low === 0) {
      slow.set(fixed);
      tally(spellCount(t), slow);
      if (onProgress && t % 2000000 === 0) onProgress(t);
    }
    const g = low * 26;
    for (let i = 0; i < 26; i++) c[i] = slow[i] + grp[g + i] + claims[i];
    if (t === 1) c[18]--;
    let same = true;
    for (let i = 0; i < 26; i++) if (c[i] !== v[i]) { same = false; break; }
    if (same) return { lies: t, claims: Array.from(v) };

    const nextLo = lo + 0x7F4A7C15;
    hi = (hi + 0x9E3779B9 + (nextLo > 0xFFFFFFFF ? 1 : 0)) >>> 0;
    lo = nextLo >>> 0;
    zh = hi; zl = lo;
    let xl = ((zl >>> 30) | (zh << 2)) >>> 0, xh = zh >>> 30;
    zl = (zl ^ xl) >>> 0; zh = (zh ^ xh) >>> 0;
    mul(0xBF58476D, 0x1CE4E5B9);
    xl = ((zl >>> 27) | (zh << 5)) >>> 0; xh = zh >>> 27;
    zl = (zl ^ xl) >>> 0; zh = (zh ^ xh) >>> 0;
    mul(0x94D049BB, 0x133111EB);
    xl = ((zl >>> 31) | (zh << 1)) >>> 0; xh = zh >>> 31;
    zl = (zl ^ xl) >>> 0; zh = (zh ^ xh) >>> 0;

    for (let i = 0; i < 26; i++) {
      const bits = i < 16 ? (zl >>> (i * 2)) & 3 : (zh >>> ((i - 16) * 2)) & 3;
      if (bits === 0) {
        const n = c[i] >= 100 ? 99 : c[i];
        const old = v[i];
        if (n !== old) {
          for (let j = 0; j < 26; j++) claims[j] += num[n * 26 + j] - num[old * 26 + j];
          v[i] = n;
        }
      }
    }
  }
}
if (typeof module !== "undefined") module.exports = { walk, spellCount, tally };
