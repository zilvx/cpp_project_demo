#!/usr/bin/env python3
"""
Mock HTTP Server — 模拟 TableDataServer (main_bs)
完全复刻原 C++ 代码中的 JSON 数据结构和路由

路由:
  GET /api/table  → JSON（与 TableDataServer::registerRoutes 完全一致）
  GET /health      → 健康检查
"""
import json
import sys
from http.server import HTTPServer, BaseHTTPRequestHandler
from datetime import datetime, timezone

# ============================================================
#  Mock 数据 — 与 TableDataServer 构造函数完全一致
#  (src/bs/TableDataServer.cpp:18-42)
# ============================================================
TABLE_DATA = {
    "columns": ["姓名", "年龄", "部门", "职位", "入职日期", "备注"],
    "editable": [True, True, False, True, True, True],
    "rows": [
        ["HappyWealthy", "28", "研发部", "高级工程师", "2022-03-15", "技术骨干"],
        ["李四", "35", "产品部", "产品经理", "2021-07-01", ""],
        ["王五", "24", "设计部", "UI 设计师", "2023-01-10", "实习生转正"],
        ["赵六", "42", "管理层", "技术总监", "2018-05-20", "部门负责人"],
        ["孙七", "30", "测试部", "测试工程师", "2022-09-01", ""],
        ["周八", "26", "运维部", "运维工程师", "2023-06-01", ""],
        ["吴九", "33", "市场部", "市场经理", "2020-11-15", "部门骨干"],
        ["郑十", "29", "研发部", "前端工程师", "2024-01-10", ""],
    ],
}


class MockTableDataHandler(BaseHTTPRequestHandler):
    """复刻 TableDataServer 的 HTTP 处理逻辑"""

    def log_message(self, fmt, *args):
        """模拟 TableDataServer 的 requestReceived 信号日志"""
        client_ip = self.client_address[0]
        method = self.command
        path = self.path
        print(f"[{client_ip}] {method} {path}")

    def _send_json(self, data, status=200):
        """复刻 res.set_content(json, 'application/json')"""
        body = json.dumps(data, ensure_ascii=False, indent=2).encode("utf-8")
        self.send_response(status)
        self.send_header("Access-Control-Allow-Origin", "*")  # 复刻 CORS 头
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        # ---- GET /api/table (JSON) ----
        # 复刻 TableDataServer.cpp:101-112
        if self.path == "/api/table":
            self._send_json(TABLE_DATA)

        # ---- GET /health ----
        # 复刻 TableDataServer.cpp:136-155
        elif self.path == "/health":
            health = {
                "status": "ok",
                "time": datetime.now(timezone.utc).isoformat(),
                "rows": len(TABLE_DATA["rows"]),
            }
            self._send_json(health)

        else:
            self._send_json({"error": "not found"}, status=404)


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    server = HTTPServer(("0.0.0.0", port), MockTableDataHandler)
    print("=" * 50)
    print("  Mock TableDataServer — Running")
    print("=" * 50)
    print(f"  GET http://localhost:{port}/api/table")
    print(f"  GET http://localhost:{port}/health")
    print("=" * 50)
    print("  Press Ctrl+C to stop\n")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nServer stopped.")
        server.shutdown()


if __name__ == "__main__":
    main()