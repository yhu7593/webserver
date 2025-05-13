/**
 * TinyWebServer 前端交互脚本
 * 提供通用的UI交互功能和动画效果
 */

// 页面加载完成后执行
document.addEventListener('DOMContentLoaded', function() {
    // 初始化所有按钮的悬停效果
    initButtonHoverEffects();
    
    // 初始化表单输入框的焦点效果
    initInputFocusEffects();
    
    // 初始化页面加载动画
    initPageLoadAnimation();
});

/**
 * 初始化按钮悬停效果
 */
function initButtonHoverEffects() {
    const buttons = document.querySelectorAll('button');
    
    buttons.forEach(button => {
        // 鼠标悬停效果
        button.addEventListener('mouseenter', function() {
            this.style.transform = 'translateY(-2px)';
            this.style.boxShadow = '0 4px 8px rgba(0,0,0,0.1)';
        });
        
        // 鼠标离开效果
        button.addEventListener('mouseleave', function() {
            this.style.transform = 'translateY(0)';
            this.style.boxShadow = 'none';
        });
        
        // 点击效果
        button.addEventListener('mousedown', function() {
            this.style.transform = 'translateY(1px)';
        });
        
        button.addEventListener('mouseup', function() {
            this.style.transform = 'translateY(-2px)';
        });
    });
}

/**
 * 初始化输入框焦点效果
 */
function initInputFocusEffects() {
    const inputs = document.querySelectorAll('input');
    
    inputs.forEach(input => {
        // 获取焦点时添加特殊样式
        input.addEventListener('focus', function() {
            this.style.borderColor = '#4a90e2';
            this.style.boxShadow = '0 0 0 3px rgba(74, 144, 226, 0.1)';
        });
        
        // 失去焦点时恢复样式
        input.addEventListener('blur', function() {
            this.style.borderColor = '';
            this.style.boxShadow = '';
        });
    });
}

/**
 * 初始化页面加载动画
 */
function initPageLoadAnimation() {
    // 获取所有主要元素
    const card = document.querySelector('.card');
    const title = document.querySelector('.title');
    
    // 如果页面包含卡片元素，添加淡入动画
    if (card) {
        card.style.opacity = '0';
        card.style.transform = 'translateY(20px)';
        card.style.transition = 'opacity 0.5s ease, transform 0.5s ease';
        
        setTimeout(() => {
            card.style.opacity = '1';
            card.style.transform = 'translateY(0)';
        }, 100);
    }
    
    // 如果页面包含标题元素，添加淡入动画
    if (title) {
        title.style.opacity = '0';
        title.style.transform = 'translateY(10px)';
        title.style.transition = 'opacity 0.5s ease 0.2s, transform 0.5s ease 0.2s';
        
        setTimeout(() => {
            title.style.opacity = '1';
            title.style.transform = 'translateY(0)';
        }, 200);
    }
}