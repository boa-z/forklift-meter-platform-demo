"""验证源码摘要的跨平台顺序及内容、路径变更检测。"""
import hashlib
from pathlib import Path
import tempfile
import unittest
from capture_screenshots import source_digest


class SourceDigestTest(unittest.TestCase):
    def test_case_sensitive_path_order(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            # 固定期望顺序，不使用宿主 Path 排序生成测试期望。
            entries = [('assets/LICENSES/a.txt', b'license'),
                       ('assets/fonts.json', b'font'),
                       ('ui/Z.c', b'upper'), ('ui/a.c', b'lower')]
            expected = hashlib.sha256()
            for name, content in reversed(entries):
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(content)
            for name, content in entries:
                expected.update(name.encode('utf-8') + bytes([0]))
                expected.update(content)
            self.assertEqual(expected.hexdigest(), source_digest(root))

    def test_source_bytes_and_names_are_bound(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / 'ui').mkdir()
            path = root / 'ui/a.c'
            path.write_bytes(b'original')
            original = source_digest(root)
            path.write_bytes(b'changed')
            self.assertNotEqual(original, source_digest(root))
            path.write_bytes(b'original')
            path.rename(root / 'ui/b.c')
            self.assertNotEqual(original, source_digest(root))


    def test_build_artifacts_do_not_change_the_digest(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / 'ui').mkdir()
            (root / 'ui/a.c').write_bytes(b'source')
            original = source_digest(root)
            # 构建残留（对象文件与字节码）不是源码，进出源码树都不得改变证据。
            (root / 'ui/a.o').write_bytes(b'object')
            (root / 'ui/libmeter.a').write_bytes(b'archive')
            (root / 'ui/__pycache__').mkdir()
            (root / 'ui/__pycache__/a.cpython-313.pyc').write_bytes(b'bytecode')
            self.assertEqual(original, source_digest(root))


if __name__ == '__main__':
    unittest.main()
