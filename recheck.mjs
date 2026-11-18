// Walks the same path as honest.c with nothing shared but the random numbers:
// writes every sentence out in full and counts its letters from the text.
const ONES = ["", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve",
  "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"];
const TENS = ["", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"];
const under100 = (n) => (n < 20 ? ONES[n] : n % 10 ? `${TENS[n / 10 | 0]}-${ONES[n % 10]}` : TENS[n / 10]);
const group = (n) => (n >= 100 ? `${ONES[n / 100 | 0]} hundred${n % 100 ? " " + under100(n % 100) : ""}` : under100(n));
const cache = Array.from({ length: 1000 }, (_, n) => group(n));
const claimCache = Array.from({ length: 100 }, (_, n) => under100(n));

function spellLies(n) {
  if (n === 0) return "zero";
  const parts = [];
  const m = Math.floor(n / 1e6), k = Math.floor(n / 1e3) % 1000, o = n % 1000;
  if (m) parts.push(cache[m] + " million");
  if (k) parts.push(cache[k] + " thousand");
  if (o) parts.push(cache[o]);
  return parts.join(", ");
}

const LETTERS = "abcdefghijklmnopqrstuvwxyz";
function sentence(t, v) {
  let s = `Thanks for the journey, Chuck: on your Mac mini, this sentence told exactly ${spellLies(t)} ${t === 1 ? "lie" : "lies"} before it could truthfully say that it has`;
  for (let i = 0; i < 26; i++) {
    s += (i === 0 ? " " : i === 25 ? ", and " : ", ") + claimCache[v[i]] + " " + LETTERS[i] + (v[i] === 1 ? "" : "'s");
  }
  return s + ".";
}

let state = 20261105n;
const M = (1n << 64n) - 1n;
function rng() {
  state = (state + 0x9E3779B97F4A7C15n) & M;
  let z = state;
  z = ((z ^ (z >> 30n)) * 0xBF58476D1CE4E5B9n) & M;
  z = ((z ^ (z >> 27n)) * 0x94D049BB133111EBn) & M;
  return z ^ (z >> 31n);
}

const v = new Array(26).fill(1);
const c = new Int32Array(26);
const limit = Number(process.argv[2]);
for (let t = 0; t <= limit; t++) {
  const s = sentence(t, v);
  c.fill(0);
  for (let j = 0; j < s.length; j++) {
    const code = s.charCodeAt(j) | 32;
    if (code >= 97 && code <= 122) c[code - 97]++;
  }
  let lies = false;
  for (let i = 0; i < 26; i++) if (c[i] !== v[i]) { lies = true; break; }
  if (!lies) {
    console.log(`first true sentence after ${t} false ones`);
    console.log(s);
    process.exit(0);
  }
  const r = rng();
  const lo = Number(r & 0xFFFFFFFFn), hi = Number((r >> 32n) & 0xFFFFFFFFn);
  for (let i = 0; i < 26; i++) {
    const bits = i < 16 ? (lo >>> (i * 2)) & 3 : (hi >>> ((i - 16) * 2)) & 3;
    if (bits === 0) v[i] = c[i] >= 100 ? 99 : c[i];
  }
  if (t % 10000000 === 0 && t) console.error(`${t} sentences checked`);
}
console.log("no true sentence found");
