// ==UserScript==
// @name         Educoder 测试结果提取器 v2
// @namespace    http://tampermonkey.net/
// @version      2.0
// @description  一键提取 Educoder 平台测试结果，通过比较预期/实际输出判断通过
// @match        *://www.educoder.net/*
// @grant        GM_setClipboard
// ==/UserScript==

(function() {
    'use strict';

    const btn = document.createElement('button');
    btn.textContent = '📋 提取测试结果';
    btn.style.cssText = 'position:fixed;top:10px;right:10px;z-index:99999;padding:10px 20px;background:#1890ff;color:#fff;border:none;border-radius:6px;cursor:pointer;font-size:14px;box-shadow:0 2px 8px rgba(0,0,0,0.3);';
    document.body.appendChild(btn);

    btn.addEventListener('click', () => {
        // 找所有测试集项（尝试多种选择器）
        let items = document.querySelectorAll('[class*="test-case-item"]');
        if (!items.length) items = document.querySelectorAll('.ant-collapse-item');
        if (!items.length) {
            alert('未找到测试用例，请先展开所有测试集');
            return;
        }

        let pass = 0, fail = 0;
        const fails = [];
        const passes = [];

        items.forEach((item, idx) => {
            const num = idx + 1;

            // 提取所有文本内容
            const allText = item.innerText || item.textContent || '';

            // 尝试从文本中提取预期和实际输出
            let expected = '', actual = '', testInput = '';

            // 方法1: 查找 "预期输出" 和 "实际输出" 标签
            const panels = item.querySelectorAll('[class*="diff-panel"]');
            if (panels.length >= 1) {
                const lastPanel = panels[panels.length - 1];
                const children = lastPanel.children;
                if (children.length >= 2) {
                    expected = children[0].innerText.trim();
                    actual = children[1].innerText.trim();
                }
            }

            // 方法2: 如果没找到，从完整文本中按模式提取
            if (!expected && !actual) {
                const m = allText.match(/预期输出[：:]\s*([\s\S]*?)(?:实际输出|$)/i);
                if (m) expected = m[1].trim();
                const m2 = allText.match(/实际输出[：:]\s*([\s\S]*?)$/i);
                if (m2) actual = m2[1].trim();
            }

            // 方法3: 查找 ins/del 标签
            if (!expected && !actual) {
                const insEls = item.querySelectorAll('ins');
                const delEls = item.querySelectorAll('del');
                if (insEls.length) expected = Array.from(insEls).map(e => e.textContent).join('');
                if (delEls.length) actual = Array.from(delEls).map(e => e.textContent).join('');
            }

            // 判断是否通过：比较预期和实际输出
            // 清理输出（去掉多余空白）
            const cleanExp = expected.replace(/\s+/g, ' ').trim();
            const cleanAct = actual.replace(/\s+/g, ' ').trim();

            // 如果没有展开（没提取到数据），检查图标颜色
            let isPassed = false;
            if (cleanExp && cleanAct) {
                isPassed = (cleanExp === cleanAct);
            } else {
                // fallback: 检查颜色或图标
                const style = item.querySelector('[class*="test-title"]');
                if (style) {
                    const color = window.getComputedStyle(style).color;
                    if (color.includes('82, 196, 26') || color.includes('52, 199, 89')) isPassed = true;
                }
                // 还是 fallback: 检查是否有绿色图标
                const svgs = item.querySelectorAll('svg, img');
                svgs.forEach(s => {
                    const fill = s.getAttribute('fill') || s.style.color || '';
                    if (fill.includes('#52c41a') || fill.includes('green') || fill.includes('#19CB70')) isPassed = true;
                });
            }

            if (isPassed) {
                pass++;
                passes.push(num);
            } else {
                fail++;
                const titleEl = item.querySelector('[class*="test-title"]');
                const title = titleEl ? titleEl.textContent.trim() : `测试集${num}`;
                fails.push({
                    num, title,
                    expected: cleanExp.substring(0, 200),
                    actual: cleanAct.substring(0, 500)
                });
            }
        });

        // 格式化
        let output = `## 测试结果: ${pass}/${pass+fail}\n\n`;
        output += `通过: ${pass}, 失败: ${fail}\n\n`;

        if (fails.length > 0) {
            output += `### 失败详情 (仅显示不匹配的测试)\n\n`;
            fails.forEach(f => {
                output += `#### ${f.title}\n`;
                if (f.expected) output += `- 预期: \`${f.expected}\`\n`;
                if (f.actual) output += `- 实际: \`${f.actual}\`\n`;
                if (!f.expected && !f.actual) output += `- (未展开，无法提取详情)\n`;
                output += `\n`;
            });
        }

        // 复制到剪贴板
        if (typeof GM_setClipboard !== 'undefined') {
            GM_setClipboard(output, 'text');
        } else {
            navigator.clipboard.writeText(output).catch(() => {});
        }

        // 显示在新窗口
        const w = window.open('', '_blank', 'width=800,height=600');
        w.document.write(`<pre style="white-space:pre-wrap;font-family:monospace;font-size:13px;padding:20px;">${output.replace(/</g,'&lt;')}</pre>`);
        w.document.title = `测试结果 ${pass}/${pass+fail}`;

        btn.textContent = `✅ ${pass}/${pass+fail}`;
        setTimeout(() => btn.textContent = '📋 提取测试结果', 5000);
    });
})();
