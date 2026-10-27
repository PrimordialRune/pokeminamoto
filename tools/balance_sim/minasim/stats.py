"""Small statistics helpers: Wilson intervals, paired differences, BH correction, robust z."""
import math
import statistics

Z95 = 1.959964


def wilson(k, n, z=Z95):
    """Wilson score interval for k successes of n; (0, 1) when n == 0."""
    if n <= 0:
        return (0.0, 1.0)
    p = k / n
    d = 1 + z * z / n
    c = p + z * z / (2 * n)
    h = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n))
    return (max(0.0, (c - h) / d), min(1.0, (c + h) / d))


def rate(k, n):
    return k / n if n else 0.0


def paired_diff(a, b, z=Z95):
    """Mean difference of paired 0/1 (or float) outcomes a-b with a normal CI and a z statistic."""
    n = min(len(a), len(b))
    if n < 2:
        return 0.0, (-1.0, 1.0), 0.0
    d = [a[i] - b[i] for i in range(n)]
    m = sum(d) / n
    sd = statistics.pstdev(d) * math.sqrt(n / (n - 1))
    se = sd / math.sqrt(n) if sd > 0 else 1e-9
    return m, (m - z * se, m + z * se), m / se


def norm_sf(x):
    return 0.5 * math.erfc(x / math.sqrt(2))


def p_from_z(zv):
    return 2 * norm_sf(abs(zv))


def bh(pvals, q=0.05):
    """Benjamini-Hochberg: returns list of booleans (discovery) in the input order."""
    n = len(pvals)
    order = sorted(range(n), key=lambda i: pvals[i])
    cut = -1
    for rank, i in enumerate(order, 1):
        if pvals[i] <= q * rank / n:
            cut = rank
    out = [False] * n
    for rank, i in enumerate(order, 1):
        if rank <= cut:
            out[i] = True
    return out


def robust_z(values):
    """Median/MAD z-scores (1.4826 scaling); zeros when the spread is degenerate."""
    if len(values) < 3:
        return [0.0] * len(values)
    med = statistics.median(values)
    mad = statistics.median([abs(v - med) for v in values]) * 1.4826
    if mad < 1e-9:
        sd = statistics.pstdev(values)
        mad = sd if sd > 1e-9 else 1.0
    return [(v - med) / mad for v in values]


def mean_ci(xs, z=Z95):
    n = len(xs)
    if n == 0:
        return 0.0, (0.0, 0.0)
    m = sum(xs) / n
    if n < 2:
        return m, (m, m)
    se = statistics.stdev(xs) / math.sqrt(n)
    return m, (m - z * se, m + z * se)


def band_status(value, lo, hi):
    return "low" if value < lo else "high" if value > hi else "ok"
