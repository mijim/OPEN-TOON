import QtQuick

Item {
    id: icon
    property string tool: ""
    property color ink: "#d5d5d5"
    implicitWidth: 22
    implicitHeight: 22
    onToolChanged: artwork.requestPaint()
    onInkChanged: artwork.requestPaint()

    Canvas {
        id: artwork
        anchors.fill: parent
        renderTarget: Canvas.Image
        onPaint: {
            const ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            ctx.save();
            ctx.scale(width / 24, height / 24);
            ctx.strokeStyle = String(icon.ink);
            ctx.fillStyle = String(icon.ink);
            ctx.lineWidth = 1.8;
            ctx.lineCap = "round";
            ctx.lineJoin = "round";

            function line(x1, y1, x2, y2) {
                ctx.beginPath();
                ctx.moveTo(x1, y1);
                ctx.lineTo(x2, y2);
                ctx.stroke();
            }
            function polygon(points, fill) {
                ctx.beginPath();
                ctx.moveTo(points[0], points[1]);
                for (let i = 2; i < points.length; i += 2)
                    ctx.lineTo(points[i], points[i + 1]);
                ctx.closePath();
                if (fill)
                    ctx.fill();
                ctx.stroke();
            }
            function node(x, y) {
                ctx.fillRect(x - 1.6, y - 1.6, 3.2, 3.2);
            }

            switch (icon.tool) {
            case "Select":
                polygon([5, 3, 5, 19, 9.4, 15.3, 12.4, 21, 15.1, 19.7, 12.2, 14.1, 19, 14.1], false);
                break;
            case "Animate":
                ctx.strokeRect(3.5, 4.5, 17, 15);
                node(3.5, 4.5);
                node(20.5, 4.5);
                node(3.5, 19.5);
                node(20.5, 19.5);
                polygon([12, 7.5, 16, 12, 12, 16.5, 8, 12], false);
                break;
            case "Camera":
                ctx.strokeRect(3, 6, 18, 13);
                line(7, 6, 9, 3); line(9, 3, 15, 3); line(15, 3, 17, 6);
                ctx.beginPath(); ctx.arc(12, 12.5, 3.5, 0, Math.PI * 2); ctx.stroke();
                node(3, 6); node(21, 6); node(3, 19); node(21, 19);
                break;
            case "Marquee":
                line(3, 5, 7, 5); line(10, 5, 14, 5); line(17, 5, 21, 5);
                line(21, 8, 21, 12); line(21, 15, 21, 19);
                line(17, 19, 13, 19); line(10, 19, 6, 19);
                line(3, 16, 3, 12); line(3, 9, 3, 5);
                break;
            case "Lasso":
                ctx.beginPath();
                ctx.moveTo(18, 5);
                ctx.bezierCurveTo(12, 2, 4, 5, 4, 12);
                ctx.bezierCurveTo(4, 19, 17, 20, 20, 13);
                ctx.bezierCurveTo(22, 8, 17, 6, 14, 10);
                ctx.stroke();
                line(14, 10, 11, 21);
                node(11, 21);
                break;
            case "Line":
                line(5, 19, 19, 5);
                ctx.beginPath(); ctx.arc(5, 19, 2.1, 0, Math.PI * 2); ctx.stroke();
                ctx.beginPath(); ctx.arc(19, 5, 2.1, 0, Math.PI * 2); ctx.stroke();
                break;
            case "Pencil":
                polygon([4, 20, 6.3, 14.7, 16.5, 4.5, 20.1, 8.1, 9.9, 18.3], false);
                line(6.3, 14.7, 9.9, 18.3);
                line(14.5, 6.5, 18.1, 10.1);
                line(4, 20, 8, 19);
                break;
            case "Raster ink":
                polygon([9, 3, 13, 3, 15, 14, 7, 14], false);
                line(11, 3, 11, 13);
                ctx.beginPath();
                ctx.moveTo(6, 15); ctx.lineTo(16, 15); ctx.lineTo(18, 19);
                ctx.quadraticCurveTo(12, 22, 5, 19); ctx.closePath(); ctx.stroke();
                line(7, 19, 7, 21); line(11, 20, 11, 22);
                break;
            case "Eraser":
                polygon([3, 16, 13, 5, 21, 12, 11, 22], false);
                line(7, 12, 15, 19);
                line(12, 22, 21, 22);
                break;
            case "Rectangle":
                ctx.strokeRect(4, 5, 16, 14);
                node(4, 5); node(20, 5); node(4, 19); node(20, 19);
                break;
            case "Ellipse":
                ctx.beginPath();
                ctx.moveTo(20, 12);
                ctx.bezierCurveTo(20, 21, 4, 21, 4, 12);
                ctx.bezierCurveTo(4, 3, 20, 3, 20, 12);
                ctx.stroke();
                break;
            case "Recolor":
                polygon([6, 5, 13, 3, 20, 11, 12, 18, 4, 10], false);
                line(7, 13, 17, 13);
                ctx.beginPath();
                ctx.moveTo(19, 16); ctx.quadraticCurveTo(16, 20, 19, 22);
                ctx.quadraticCurveTo(22, 20, 19, 16); ctx.stroke();
                break;
            case "Edit points":
                ctx.beginPath();
                ctx.moveTo(4, 19);
                ctx.bezierCurveTo(7, 5, 17, 19, 20, 5);
                ctx.stroke();
                line(4, 19, 7, 8); line(17, 16, 20, 5);
                node(4, 19); node(7, 8); node(17, 16); node(20, 5);
                break;
            }
            ctx.restore();
        }
    }
}
