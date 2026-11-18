const { walk } = require("./walk.js");
const MIDDLE = "before it could truthfully say that it has";
let start = Date.now();
const r = walk("Thanks for the journey, Chuck: on your Mac mini, this sentence told exactly", MIDDLE, 0, 20261105);
console.log("official", r.lies, r.claims.join(","), (Date.now() - start) / 1000 + "s");
start = Date.now();
const e = walk("Thanks for the pancakes: on a Saturday morning, this sentence told exactly", MIDDLE, 0, 7);
console.log("example", e.lies, e.claims.join(","), (Date.now() - start) / 1000 + "s");
