// ==UserScript==
// @name         EduCoder Pascal-S Auto Tester
// @namespace    http://tampermonkey.net/
// @version      1.0
// @description  自动提取头歌平台 Pascal-S 编译器的评测结果并导出
// @author       Pascal-S Team
// @match        *://*.educoder.net/*
// @grant        GM_setClipboard
// ==/UserScript==

(function() {
    'use strict';

    // 创建控制面板
    const panel = document.createElement('div');
    panel.style.position = 'fixed';
    panel.style.bottom = '20px';
    panel.style.right = '20px';
    panel.style.padding = '10px';
    panel.style.background = '#333';
    panel.style.color = 'white';
    panel.style.zIndex = '9999';
    panel.style.borderRadius = '5px';
    panel.style.boxShadow = '0 0 10px rgba(0,0,0,0.5)';
    
    const title = document.createElement('div');
    title.innerText = 'Pascal-S 测试提取器';
    title.style.fontWeight = 'bold';
    title.style.marginBottom = '10px';
    panel.appendChild(title);

    const extractBtn = document.createElement('button');
    extractBtn.innerText = '提取测试结果';
    extractBtn.style.padding = '5px 10px';
    extractBtn.style.cursor = 'pointer';
    
    extractBtn.onclick = function() {
        // 查找所有测试用例的结果元素 (根据平台实际DOM结构调整选择器)
        const resultElements = document.querySelectorAll('.test-case-result');
        if (resultElements.length === 0) {
            alert('未找到评测结果，请等待评测完毕或确认页面结构。');
            return;
        }

        let passed = 0;
        let total = resultElements.length;
        let report = "# Pascal-S 评测结果提取\n\n";

        resultElements.forEach((el, index) => {
            const caseName = el.querySelector('.case-name')?.innerText || `用例 ${index + 1}`;
            const status = el.querySelector('.status')?.innerText || '未知';
            const log = el.querySelector('.error-log')?.innerText || '无错误日志';
            
            if (status.includes('通过') || status.includes('Pass')) {
                passed++;
                report += `- ✅ **${caseName}**: 通过\n`;
            } else {
                report += `- ❌ **${caseName}**: 失败\n`;
                report += `  - 日志: \`${log.substring(0, 100).replace(/\n/g, ' ')}...\`\n`;
            }
        });

        report += `\n**总计**: ${passed} / ${total} (${((passed/total)*100).toFixed(1)}%)\n`;
        
        // 复制到剪贴板
        GM_setClipboard(report);
        alert(`提取成功！共 ${total} 个用例，通过 ${passed} 个。\nMarkdown 报告已复制到剪贴板，可直接粘贴到文档中。`);
        
        console.log("----- 评测结果 -----");
        console.log(report);
    };

    panel.appendChild(extractBtn);
    document.body.appendChild(panel);
})();
