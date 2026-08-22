import QtQuick
import QtQuick.Layouts
import "../Theme.js" as Theme

// I/Q 星座图（对应设计稿 .constellation svg，Canvas 自绘）
Item {
    id: root
    property var points: []
    property string label: "QAM 64"
    property string rateText: "50 MHz"
    property bool previewing: false
    property int pointRadius: 3

    onPointsChanged: cvs.requestPaint()

    Canvas {
        id: cvs
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            var w = cvs.width, h = cvs.height;
            var cx = w / 2, cy = h / 2;
            var scale = Math.min(w, h) / 2 - 12;
            var pts = root.points;

            // 网格：I/Q 轴 + 同心圆
            ctx.strokeStyle = Theme.colors.constGrid;
            ctx.lineWidth = 0.6;
            ctx.beginPath();
            ctx.moveTo(cx, 6); ctx.lineTo(cx, h - 6);
            ctx.moveTo(6, cy); ctx.lineTo(w - 6, cy);
            ctx.stroke();
            ctx.beginPath(); ctx.arc(cx, cy, scale * 0.55, 0, 2 * Math.PI); ctx.stroke();
            ctx.beginPath(); ctx.arc(cx, cy, scale * 0.35, 0, 2 * Math.PI); ctx.stroke();

            // 星座点
            var r = root.pointRadius;
            var accent = -1, minD = 1e9;
            ctx.fillStyle = Theme.colors.constPoint;
            ctx.globalAlpha = 0.8;
            for (var i = 0; i < pts.length; i++) {
                var px = cx + pts[i].x * scale;
                var py = cy - pts[i].y * scale;
                ctx.beginPath(); ctx.arc(px, py, r, 0, 2 * Math.PI); ctx.fill();
                var d = Math.abs(pts[i].x) + Math.abs(pts[i].y);
                if (d < minD) { minD = d; accent = i; }
            }
            // 高亮最靠近原点的点（对应设计稿中的蓝色点）
            if (accent >= 0 && pts.length > 0) {
                ctx.fillStyle = Theme.colors.constAccent;
                ctx.globalAlpha = 1;
                ctx.beginPath();
                ctx.arc(cx + pts[accent].x * scale, cy - pts[accent].y * scale, r + 0.5, 0, 2 * Math.PI);
                ctx.fill();
            }
            ctx.globalAlpha = 1;
        }
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    // I/Q 轴标签
    Text {
        text: "Q"
        color: Theme.colors.navIcon
        font.pixelSize: 9
        x: parent.width / 2 + 3
        y: 2
    }
    Text {
        text: "I"
        color: Theme.colors.navIcon
        font.pixelSize: 9
        anchors.right: parent.right
        anchors.rightMargin: 3
        anchors.verticalCenter: parent.verticalCenter
    }

    // 底部说明（格式 + 符号率）
    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 6
        anchors.rightMargin: 6
        anchors.bottomMargin: 4
        Text {
            text: root.label
            color: Theme.colors.unit
            font.pixelSize: 10
            font.weight: Font.Medium
        }
        Item { Layout.fillWidth: true }
        Text {
            text: root.rateText
            color: Theme.colors.unit
            font.pixelSize: 10
        }
    }

    // 预览遮罩
    Rectangle {
        anchors.fill: parent
        color: Theme.colors.constBg
        opacity: root.previewing ? 0.55 : 0
        visible: root.previewing
        Text {
            anchors.centerIn: parent
            text: "预览中…"
            color: Theme.colors.appIconText
            font.pixelSize: 14
            font.bold: true
        }
    }
    Behavior on opacity { NumberAnimation { duration: 200 } }
}