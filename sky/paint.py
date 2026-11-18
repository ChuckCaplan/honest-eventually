# Turns a plane file into the picture. Brightness comes from how many sentences landed on a pixel,
# color from how false the sentences around it were, and the star marks where the true one landed.
import sys, numpy as np
from scipy.ndimage import gaussian_filter
from PIL import Image
src, out = sys.argv[1], sys.argv[2]
with open(src, "rb") as f:
    size, tx, ty = np.frombuffer(f.read(24), dtype=np.uint64).astype(int)
    n = np.frombuffer(f.read(4 * size * size), dtype=np.uint32).reshape(size, size).astype(np.float64)
    wrong = np.frombuffer(f.read(8 * size * size), dtype=np.uint64).reshape(size, size).astype(np.float64)
print("sentences plotted:", int(n.sum()), "pixels lit:", int((n > 0).sum()), "of", size * size, "true sentence at", tx, ty)
s = size / 1400
tone = np.clip(np.log1p(n) / np.log1p(np.percentile(n[n > 0], 99.8)), 0, 1) ** 1.05
near = gaussian_filter(n, 14 * s)
mean = np.divide(gaussian_filter(wrong, 14 * s), near, out=np.full_like(n, 12.0), where=near > 1e-3)
busy = near > np.percentile(near[near > 0], 50)
lo, hi = np.percentile(mean[busy], [3, 97])
print("average false claims nearby, 3rd to 97th percentile:", round(lo, 2), round(hi, 2))
k = np.clip((hi - mean) / (hi - lo), 0, 1)[..., None]
stops = np.array([[0.24, 0.34, 1.0], [1.0, 0.33, 0.56], [1.0, 0.82, 0.42]])
color = np.where(k < .5, stops[0] * (1 - 2 * k) + stops[1] * 2 * k, stops[1] * (2 - 2 * k) + stops[2] * (2 * k - 1))
rgb = color * tone[..., None]
rgb += np.array([1.0, 0.96, 0.9]) * tone[..., None] ** 6 * 0.5
glow = np.stack([gaussian_filter(rgb[..., c], 9 * s) * .6 + gaussian_filter(rgb[..., c], 42 * s) * .5 for c in range(3)], axis=-1)
rgb = rgb + glow + np.array([0.010, 0.014, 0.034])
yy, xx = np.mgrid[0:size, 0:size]
dx, dy = np.abs(xx - tx).astype(np.float64), np.abs(yy - ty).astype(np.float64)
d = np.hypot(dx, dy)
flare = np.exp(-dx / (24 * s)) * np.exp(-(dy / (1.1 * s)) ** 2) + np.exp(-dy / (24 * s)) * np.exp(-(dx / (1.1 * s)) ** 2)
star = np.exp(-(d / (5 * s)) ** 2) + 0.28 * np.exp(-(d / (17 * s)) ** 2) + 0.9 * flare
rgb += np.array([1.0, 0.87, 0.28]) * star[..., None]
rgb += np.array([1.0, 1.0, 0.9]) * (np.exp(-(d / (2.2 * s)) ** 2))[..., None]
img = Image.fromarray((np.clip(rgb, 0, 1) * 255).astype(np.uint8))
img.save(out + ".png")
img.save(out + ".jpg", quality=93, subsampling=0)
img.resize((1400, 1400), Image.LANCZOS).save(out + "-small.png")
