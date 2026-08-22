// 星座图点数计算与 EVM 抖动（对应设计稿 QAM/PSK/FSK/ASK 调制格式）
.pragma library

// 生成 {1..max} 与其负数构成的电平数组
function _levels(max, inc) {
    var l = [];
    for (var v = 1; v <= max; v += inc) {
        l.push(v);
        l.push(-v);
    }
    return l;
}

function _squareQam(levels, max) {
    var pts = [];
    for (var i = 0; i < levels.length; i++)
        for (var j = 0; j < levels.length; j++)
            pts.push({ x: levels[i] / max * 0.85, y: levels[j] / max * 0.85 });
    return pts;
}

// 依据调制格式返回归一化星座点（坐标范围约 -0.85 ~ 0.85）
function pointsForFormat(format) {
    if (format === "QPSK") {
        var s = 0.85 * 0.70710678;
        return [
            { x:  s, y:  s }, { x: -s, y:  s },
            { x: -s, y: -s }, { x:  s, y: -s },
        ];
    }
    if (format === "16QAM") return _squareQam(_levels(3, 2), 3);
    if (format === "QAM 256") return _squareQam(_levels(15, 2), 15);
    if (format === "FSK") {
        // FSK 以圆周上的点表示不同频偏音调
        var pts = [], n = 8;
        for (var k = 0; k < n; k++) {
            var a = 2 * Math.PI * k / n;
            pts.push({ x: 0.85 * Math.cos(a), y: 0.85 * Math.sin(a) });
        }
        return pts;
    }
    if (format === "ASK") {
        // ASK 电平落在 I 轴上
        var l = _levels(3, 2), out = [];
        for (var a2 = 0; a2 < l.length; a2++) out.push({ x: l[a2] / 3 * 0.85, y: 0 });
        return out;
    }
    // 默认 QAM 64
    return _squareQam(_levels(7, 2), 7);
}

// Box-Muller 高斯噪声
function gauss() {
    var u = 0, v = 0;
    while (u === 0) u = Math.random();
    while (v === 0) v = Math.random();
    return Math.sqrt(-2.0 * Math.log(u)) * Math.cos(2.0 * Math.PI * v);
}

// 按 EVM(RMS%) 给星座点叠加抖动
function applyEVM(pts, evmPct) {
    var ref = 0;
    for (var i = 0; i < pts.length; i++)
        ref += Math.sqrt(pts[i].x * pts[i].x + pts[i].y * pts[i].y);
    ref = ref / Math.max(pts.length, 1);
    var sigma = evmPct / 100.0 * ref;
    var out = [];
    for (var j = 0; j < pts.length; j++)
        out.push({ x: pts[j].x + gauss() * sigma, y: pts[j].y + gauss() * sigma });
    return out;
}

// 按点数选择星座点半径（点越多越小，避免重叠）
function pointRadius(n) {
    if (n <= 16) return 3.5;
    if (n <= 64) return 3;
    return 1.8;
}