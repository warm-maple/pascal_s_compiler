// ==UserScript==
// @name         Educoder 测试数据提取器
// @namespace    http://tampermonkey.net/
// @version      1.0
// @description  提取所有测试集的输入和预期输出，生成 JSON 供本地测试使用
// @match        https://www.educoder.net/*
// @grant        none
// ==/UserScript==

(function() {
    'use strict';

    // 添加浮动按钮
    const btn = document.createElement('button');
    btn.textContent = '📋 提取测试数据';
    btn.style.cssText = 'position:fixed;top:10px;right:10px;z-index:99999;padding:10px 18px;background:#1890ff;color:#fff;border:none;border-radius:6px;cursor:pointer;font-size:14px;box-shadow:0 2px 8px rgba(0,0,0,.3);';
    document.body.appendChild(btn);

    btn.addEventListener('click', async () => {
        btn.textContent = '⏳ 展开中...';
        btn.disabled = true;

        // 1. 点击所有折叠的测试项以展开
        const headers = document.querySelectorAll('a.case-header___xppld');
        for (const h of headers) {
            const desc = h.nextElementSibling;
            if (desc && desc.classList.contains('hide')) {
                h.click();
                await new Promise(r => setTimeout(r, 50));
            }
        }
        await new Promise(r => setTimeout(r, 500));

        btn.textContent = '⏳ 提取中...';

        // 2. 提取数据
        const items = document.querySelectorAll('li.test-case-item___E3CU9');
        const results = [];

        items.forEach((item, idx) => {
            const titleEl = item.querySelector('span.test-title___mf3Df');
            const title = titleEl ? titleEl.textContent.trim() : `测试集${idx + 1}`;

            // 提取输入：在 diff-panel-container-2 里的 ins 元素
            const inputContainer = item.querySelector('.diff-panel-container-2___RYOLG');
            let input = '';
            if (inputContainer) {
                // 获取所有 ins 元素的文本
                const insEls = inputContainer.querySelectorAll('ins');
                input = Array.from(insEls).map(el => el.textContent).join('').trim();
            }

            // 提取预期输出：output-title-container 之后的 diff-panel-container 的第一个 div
            const outputTitle = item.querySelector('p.output-title-container___P2NjC');
            let expected = '';
            if (outputTitle) {
                const diffPanel = outputTitle.nextElementSibling;
                if (diffPanel && diffPanel.classList.contains('diff-panel-container___IpXsK')) {
                    const firstDiv = diffPanel.querySelector(':scope > div:first-child');
                    if (firstDiv) {
                        // 获取所有文本内容，包括 span 和 ins 元素
                        expected = '';
                        firstDiv.childNodes.forEach(node => {
                            if (node.nodeType === Node.TEXT_NODE) {
                                expected += node.textContent;
                            } else if (node.nodeType === Node.ELEMENT_NODE) {
                                const tag = node.tagName.toLowerCase();
                                // span 和 ins 都可能包含预期文本
                                if (tag === 'span' || tag === 'ins') {
                                    // 检查是否有 enter 类（换行标记）
                                    if (node.classList.contains('enter___UGDlZ')) {
                                        expected += '\n';
                                    } else {
                                        expected += node.textContent;
                                    }
                                }
                            }
                        });
                        expected = expected.trim();
                    }
                }
            }

            // 分离文件名和实际输入数据
            const lines = input.split('\n');
            const filename = lines[0] || '';
            const inputData = lines.slice(1).join('\n').trim();

            results.push({
                id: idx + 1,
                title: title,
                filename: filename,
                input: inputData,
                expected: expected
            });
        });

        // 3. 生成多种格式的输出

        // JSON 格式（给程序用）
        const jsonOutput = JSON.stringify(results, null, 2);

        // 简洁文本格式（给人看）
        let textOutput = `# 测试数据 (共 ${results.length} 组)\n\n`;
        results.forEach(r => {
            textOutput += `## ${r.title} — ${r.filename}\n`;
            if (r.input) {
                textOutput += `输入:\n${r.input}\n`;
            }
            textOutput += `预期: ${r.expected}\n\n`;
        });

        // 4. 显示结果弹窗
        const overlay = document.createElement('div');
        overlay.style.cssText = 'position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,.7);z-index:100000;display:flex;align-items:center;justify-content:center;';

        const dialog = document.createElement('div');
        dialog.style.cssText = 'background:#1e1e1e;color:#d4d4d4;border-radius:12px;padding:20px;width:80%;max-width:900px;max-height:80vh;display:flex;flex-direction:column;font-family:monospace;';

        const toolbar = document.createElement('div');
        toolbar.style.cssText = 'display:flex;gap:8px;margin-bottom:12px;align-items:center;';

        const copyJsonBtn = document.createElement('button');
        copyJsonBtn.textContent = '📋 复制 JSON';
        copyJsonBtn.style.cssText = 'padding:6px 14px;background:#52c41a;color:#fff;border:none;border-radius:4px;cursor:pointer;font-size:13px;';

        const copyTextBtn = document.createElement('button');
        copyTextBtn.textContent = '📋 复制文本';
        copyTextBtn.style.cssText = 'padding:6px 14px;background:#1890ff;color:#fff;border:none;border-radius:4px;cursor:pointer;font-size:13px;';

        const downloadBtn = document.createElement('button');
        downloadBtn.textContent = '💾 下载 JSON';
        downloadBtn.style.cssText = 'padding:6px 14px;background:#722ed1;color:#fff;border:none;border-radius:4px;cursor:pointer;font-size:13px;';

        const closeBtn = document.createElement('button');
        closeBtn.textContent = '✕ 关闭';
        closeBtn.style.cssText = 'padding:6px 14px;background:#ff4d4f;color:#fff;border:none;border-radius:4px;cursor:pointer;font-size:13px;margin-left:auto;';

        const info = document.createElement('span');
        info.textContent = `共 ${results.length} 组测试`;
        info.style.cssText = 'color:#888;font-size:12px;margin-left:8px;';

        toolbar.append(copyJsonBtn, copyTextBtn, downloadBtn, info, closeBtn);

        const textarea = document.createElement('textarea');
        textarea.value = textOutput;
        textarea.style.cssText = 'flex:1;background:#2d2d2d;color:#d4d4d4;border:1px solid #444;border-radius:6px;padding:12px;font-family:monospace;font-size:12px;resize:none;white-space:pre;';
        textarea.readOnly = true;

        dialog.append(toolbar, textarea);
        overlay.appendChild(dialog);
        document.body.appendChild(overlay);

        copyJsonBtn.onclick = () => {
            navigator.clipboard.writeText(jsonOutput).then(() => {
                copyJsonBtn.textContent = '✅ 已复制 JSON';
                setTimeout(() => copyJsonBtn.textContent = '📋 复制 JSON', 2000);
            });
        };

        copyTextBtn.onclick = () => {
            navigator.clipboard.writeText(textOutput).then(() => {
                copyTextBtn.textContent = '✅ 已复制文本';
                setTimeout(() => copyTextBtn.textContent = '📋 复制文本', 2000);
            });
        };

        downloadBtn.onclick = () => {
            const blob = new Blob([jsonOutput], { type: 'application/json' });
            const a = document.createElement('a');
            a.href = URL.createObjectURL(blob);
            a.download = 'test_data.json';
            a.click();
            URL.revokeObjectURL(a.href);
            downloadBtn.textContent = '✅ 已下载';
            setTimeout(() => downloadBtn.textContent = '💾 下载 JSON', 2000);
        };

        closeBtn.onclick = () => overlay.remove();
        overlay.onclick = (e) => { if (e.target === overlay) overlay.remove(); };

        btn.textContent = '📋 提取测试数据';
        btn.disabled = false;
    });
})();
