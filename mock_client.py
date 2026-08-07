#!/usr/bin/env python3
"""
Mock HTTP Client — 模拟 HttpDataProvider (main_ui)
完全复刻原 C++ 代码中的 HTTP 交互逻辑

流程:
  1. fetchData()  → 发送 GET 请求到 /api/table
  2. onReplyFinished() → 接收响应，解析 JSON
  3. parseJson()   → 将 JSON 转换为 TableData 结构体
  4. loadData()    → 模拟填充 QTableWidget（打印表格）
"""
import json
import urllib.request
import urllib.error
from dataclasses import dataclass, field


# ============================================================
#  TableData 结构体 — 复刻 src/ui/core/TableData.h
# ============================================================
@dataclass
class TableData:
    columns: list[str] = field(default_factory=list)
    rows: list[list[str]] = field(default_factory=list)
    editable: list[bool] = field(default_factory=list)

    def is_valid(self) -> bool:
        return len(self.columns) > 0

    def column_count(self) -> int:
        return len(self.columns)

    def row_count(self) -> int:
        return len(self.rows)


# ============================================================
#  HttpDataProvider 模拟 — 复刻 src/ui/providers/HttpDataProvider.cpp
# ============================================================
class MockHttpDataProvider:
    """模拟 HttpDataProvider 的 HTTP 请求和解析逻辑"""

    def __init__(self, url: str):
        self.url = url
        self.data: TableData | None = None
        self.error: str | None = None

    def fetch_data(self):
        """
        复刻 HttpDataProvider::fetchData()
        (src/ui/providers/HttpDataProvider.cpp:15-21)

        发送 GET 请求到后端，异步 → 同步模拟
        """
        req = urllib.request.Request(
            self.url,
            headers={
                "Content-Type": "application/json",
                "Accept": "application/json",
            },
        )
        try:
            with urllib.request.urlopen(req, timeout=5) as resp:
                body = resp.read()
                self.on_reply_finished(body)
        except urllib.error.URLError as e:
            self.error = f"HTTP {e.reason}"
            print(f"[ERROR] {self.error}")

    def on_reply_finished(self, body: bytes):
        """
        复刻 HttpDataProvider::onReplyFinished()
        (src/ui/providers/HttpDataProvider.cpp:23-37)

        接收响应，解析 JSON，校验数据
        """
        data = self.parse_json(body)
        if not data.is_valid():
            self.error = "JSON parse error: invalid data format"
            print(f"[ERROR] {self.error}")
            return
        self.data = data
        print("[OK] dataReady signal emitted (simulated)")

    def parse_json(self, raw: bytes) -> TableData:
        """
        复刻 HttpDataProvider::parseJson()
        (src/ui/providers/HttpDataProvider.cpp:39-62)

        将 JSON 反序列化为 TableData 结构体
        """
        result = TableData()
        try:
            doc = json.loads(raw)
        except json.JSONDecodeError as e:
            print(f"[WARN] JSON parse error at offset {e.pos}: {e.msg}")
            return result

        # 提取 columns
        for item in doc.get("columns", []):
            result.columns.append(str(item))

        # 提取 rows
        for row_arr in doc.get("rows", []):
            row = [str(cell) for cell in row_arr]
            result.rows.append(row)

        # 提取 editable
        for item in doc.get("editable", []):
            result.editable.append(bool(item))

        return result


# ============================================================
#  TableWidget 模拟 — 复刻 src/ui/widgets/TableWidget.cpp:40-59
# ============================================================
def load_data(data: TableData):
    """
    复刻 TableWidget::loadData()
    模拟 QTableWidget 的填充逻辑，打印为文本表格
    """
    if not data.is_valid():
        print("[ERROR] TableData is empty, cannot load table")
        return

    print(f"\n{'=' * 70}")
    print(f"  表格已加载 | {data.column_count()} 列 × {data.row_count()} 行")
    print(f"{'=' * 70}")

    # 打印表头
    header = " | ".join(f"{col:^12}" for col in data.columns)
    print(f"  {header}")
    print(f"  {'-' * len(header)}")

    # 打印每一行
    for i, row in enumerate(data.rows):
        cells = [f"{cell:^12}" for cell in row]
        # 标记可编辑/不可编辑列
        for j, cell in enumerate(cells):
            if j < len(data.editable) and not data.editable[j]:
                cells[j] = f"\033[90m{cell}\033[0m"  # 灰色 = 不可编辑
        print(f"  {' | '.join(cells)}")

    print(f"  {'─' * len(header)}")
    print(f"  \033[90m灰色 = 不可编辑列\033[0m")
    print(f"{'=' * 70}\n")


# ============================================================
#  main — 复刻 src/ui/main_ui.cpp 中的 HTTP 交互逻辑
# ============================================================
def main():
    import sys
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    url = f"http://localhost:{port}/api/table"

    print(f"\n{'=' * 50}")
    print(f"  Mock Qt UI — 模拟 HTTP 交互")
    print(f"{'=' * 50}")
    print(f"\n  → 创建 HttpDataProvider(QUrl(\"{url}\"))")
    print(f"  → connect(dataReady, loadData)")
    print(f"  → fetchData()\n")

    # 模拟 HttpDataProvider 调用
    provider = MockHttpDataProvider(url)
    provider.fetch_data()

    if provider.data and provider.data.is_valid():
        # 模拟 connect(dataReady → loadData)
        load_data(provider.data)
    else:
        print(f"[FAIL] 数据获取失败: {provider.error or '未知错误'}")
        print("\n提示: 请先启动 mock 服务端:")
        print(f"  python3 mock_server.py {port} &")


if __name__ == "__main__":
    main()