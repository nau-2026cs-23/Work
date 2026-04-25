// ===================== 工具函数 =====================
const DB = {
  get: k => { try { return JSON.parse(localStorage.getItem(k)) } catch { return null } },
  set: (k, v) => localStorage.setItem(k, JSON.stringify(v)),
  getArr: k => DB.get(k) || [],
  push: (k, v) => { const a = DB.getArr(k); a.push(v); DB.set(k, a); return v; }
};

function toast(msg, dur = 2000) {
  let el = document.getElementById('toast');
  if (!el) { el = document.createElement('div'); el.id = 'toast'; document.body.appendChild(el); }
  el.textContent = msg; el.classList.add('show');
  clearTimeout(el._t);
  el._t = setTimeout(() => el.classList.remove('show'), dur);
}

function uid() { return Date.now().toString(36) + Math.random().toString(36).slice(2); }
function fmtDate(ts) {
  const d = new Date(ts), now = new Date();
  const diff = now - d;
  if (diff < 60000) return '刚刚';
  if (diff < 3600000) return Math.floor(diff / 60000) + '分钟前';
  if (diff < 86400000) return Math.floor(diff / 3600000) + '小时前';
  if (diff < 604800000) return Math.floor(diff / 86400000) + '天前';
  return `${d.getMonth()+1}/${d.getDate()}`;
}
function fmtMoney(n) { return '¥' + parseFloat(n).toFixed(2); }

// ===================== 当前用户 =====================
const Auth = {
  cur: () => DB.get('cur_user'),
  login: (u) => { DB.set('cur_user', u); },
  logout: () => { localStorage.removeItem('cur_user'); },
  require: () => { if (!Auth.cur()) { location.href = 'login.html'; return false; } return true; },
  isAdmin: () => { const u = Auth.cur(); return u && u.role === 'admin'; }
};

// ===================== 初始化种子数据 =====================
function initSeedData() {
  if (DB.get('seeded')) return;

  // 管理员账号
  const users = [
    { id: 'u_admin', phone: '18800000000', email: 'admin@campus.com', password: '123456', nickname: '管理员', avatar: '', role: 'admin', verified: true, createdAt: Date.now() - 864000000, bio: '平台管理员', addresses: [] },
    { id: 'u1', phone: '13800000001', email: 'user1@campus.com', password: '123456', nickname: '小明', avatar: '', role: 'user', verified: true, createdAt: Date.now() - 432000000, bio: '爱好摄影', addresses: [{ id: 'a1', name: '小明', phone: '13800000001', address: '大学城北路1号学生公寓3栋201', isDefault: true }] },
    { id: 'u2', phone: '13800000002', email: 'user2@campus.com', password: '123456', nickname: '小红', avatar: '', role: 'user', verified: false, createdAt: Date.now() - 259200000, bio: '喜欢二手', addresses: [] }
  ];
  DB.set('users', users);

  const cates = ['全部','手机数码','服装服饰','图书教材','自行车','家居生活','运动户外','美妆护肤','其他'];
  DB.set('categories', cates);

  const conditions = ['全新','99新','95新','9成新','8成新','7成新及以下'];

  const products = [
    { id: 'p1', sellerId: 'u1', title: 'iPhone 12 128G 黑色 95新', desc: '买来没怎么用，换新款了，诚意出售。配件齐全，原装充电器。', price: 1999, origPrice: 5999, category: '手机数码', condition: '95新', images: [], location: '北京朝阳', status: 'on', views: 128, favs: 12, createdAt: Date.now() - 86400000 },
    { id: 'p2', sellerId: 'u2', title: '高等数学第七版上下册', desc: '同济版，九成新，没有划线，一起出售。', price: 25, origPrice: 68, category: '图书教材', condition: '9成新', images: [], location: '北京海淀', status: 'on', views: 56, favs: 5, createdAt: Date.now() - 172800000 },
    { id: 'p3', sellerId: 'u1', title: 'AirPods Pro 降噪耳机', desc: '成色很好，音质出众，换有线了出。', price: 899, origPrice: 1999, category: '手机数码', condition: '9成新', images: [], location: '北京朝阳', status: 'on', views: 234, favs: 31, createdAt: Date.now() - 43200000 },
    { id: 'p4', sellerId: 'u2', title: '山地自行车 美利达', desc: '骑了一年，链条已换新，速度快。', price: 350, origPrice: 900, category: '自行车', condition: '8成新', images: [], location: '北京海淀', status: 'on', views: 89, favs: 8, createdAt: Date.now() - 259200000 },
    { id: 'p5', sellerId: 'u1', title: '宿舍台灯 LED护眼', desc: '用了一学期，亮度可调，带USB充电口。', price: 30, origPrice: 89, category: '家居生活', condition: '95新', images: [], location: '北京朝阳', status: 'on', views: 44, favs: 3, createdAt: Date.now() - 345600000 },
    { id: 'p6', sellerId: 'u2', title: 'Nike Air Force 1 白色 42码', desc: '穿过几次，鞋底无磨损，附购买凭证。', price: 280, origPrice: 649, category: '服装服饰', condition: '95新', images: [], location: '北京海淀', status: 'on', views: 167, favs: 19, createdAt: Date.now() - 129600000 }
  ];
  DB.set('products', products);

  const orders = [
    { id: 'o1', buyerId: 'u2', sellerId: 'u1', productId: 'p1', price: 1999, status: 'done', shipType: '快递', shipNo: 'SF1234567890', address: '大学城北路1号', createdAt: Date.now() - 604800000, paidAt: Date.now() - 604000000, shippedAt: Date.now() - 500000000, doneAt: Date.now() - 400000000 },
    { id: 'o2', buyerId: 'u2', sellerId: 'u1', productId: 'p3', price: 899, status: 'pending_ship', shipType: '快递', address: '大学城北路1号', createdAt: Date.now() - 86400000, paidAt: Date.now() - 80000000 }
  ];
  DB.set('orders', orders);

  const reviews = [
    { id: 'r1', orderId: 'o1', fromId: 'u2', toId: 'u1', rating: 5, content: '卖家很靠谱，商品和描述一致，发货快！', createdAt: Date.now() - 300000000 },
    { id: 'r2', orderId: 'o1', fromId: 'u1', toId: 'u2', rating: 5, content: '买家很爽快，付款迅速，好评！', createdAt: Date.now() - 299000000 }
  ];
  DB.set('reviews', reviews);

  const msgs = [
    { id: 'm1', fromId: 'u2', toId: 'u1', productId: 'p1', content: '你好，这个手机还在吗？', createdAt: Date.now() - 90000000, read: true },
    { id: 'm2', fromId: 'u1', toId: 'u2', productId: 'p1', content: '在的，可以小刀吗？', createdAt: Date.now() - 89000000, read: true },
    { id: 'm3', fromId: 'u2', toId: 'u1', productId: 'p1', content: '1800可以吗？', createdAt: Date.now() - 88000000, read: false }
  ];
  DB.set('messages', msgs);

  const notifs = [
    { id: 'n1', userId: 'u2', type: 'order', content: '您的订单已创建，请尽快付款', createdAt: Date.now() - 86400000, read: false },
    { id: 'n2', userId: 'u1', type: 'order', content: '您有新订单，买家已付款，请尽快发货', createdAt: Date.now() - 80000000, read: false },
    { id: 'n3', userId: 'u1', type: 'system', content: '欢迎加入校园二手交易平台！', createdAt: Date.now() - 864000000, read: true }
  ];
  DB.set('notifications', notifs);

  const banners = [
    { id: 'b1', title: '📱 手机数码大促', subtitle: '全场二手好货', color: 'linear-gradient(135deg,#ff6b35,#ff9a56)' },
    { id: 'b2', title: '📚 教材以书换书', subtitle: '知识不浪费', color: 'linear-gradient(135deg,#3498db,#5dade2)' },
    { id: 'b3', title: '🚲 校园出行首选', subtitle: '自行车特卖会', color: 'linear-gradient(135deg,#27ae60,#58d68d)' }
  ];
  DB.set('banners', banners);

  DB.set('seeded', true);
}

// ===================== 导航栏渲染 =====================
function renderNavbar(activeSearch = '') {
  const u = Auth.cur();
  const unreadCount = u ? DB.getArr('notifications').filter(n => n.userId === u.id && !n.read).length : 0;
  const msgCount = u ? (() => {
    const msgs = DB.getArr('messages');
    const convs = getConversations(u.id);
    return convs.reduce((sum, c) => sum + (msgs.filter(m => m.toId === u.id && m.fromId === c.otherId && m.productId === c.productId && !m.read).length), 0);
  })() : 0;

  return `
  <nav class="navbar">
    <div class="navbar-inner">
      <a href="index.html" class="logo">🏫 校园二手</a>
      <form class="search-bar" onsubmit="doSearch(event)">
        <input name="q" type="text" placeholder="搜索商品..." value="${activeSearch}">
        <button type="submit">搜索</button>
      </form>
      <div class="nav-links">
        ${u ? `
          <a href="index.html">首页</a>
          <a href="orders.html">订单</a>
          <a href="chat.html" class="${msgCount > 0 ? 'badge-dot' : ''}">消息</a>
          <a href="profile.html" class="${unreadCount > 0 ? 'badge-dot' : ''}">
            ${u.avatar ? `<img src="${u.avatar}" class="avatar-sm">` : `<span style="font-size:26px">${u.nickname.charAt(0)}</span>`}
          </a>
          <a href="publish.html" class="btn-publish">+ 发布</a>
        ` : `
          <a href="login.html">登录</a>
          <a href="login.html?mode=reg">注册</a>
        `}
        ${Auth.isAdmin() ? '<a href="admin.html" style="color:var(--primary)">后台</a>' : ''}
      </div>
    </div>
  </nav>
  <div class="bottom-nav">
    <div class="bottom-nav-inner">
      <div class="bottom-nav-item" onclick="location.href='index.html'"><span class="icon">🏠</span>首页</div>
      <div class="bottom-nav-item" onclick="location.href='index.html?tab=search'"><span class="icon">🔍</span>搜索</div>
      <div class="bottom-nav-item" onclick="authGo('publish.html')"><span class="icon">➕</span>发布</div>
      <div class="bottom-nav-item" onclick="authGo('orders.html')"><span class="icon">📦</span>订单</div>
      <div class="bottom-nav-item" onclick="authGo('profile.html')"><span class="icon">👤</span>我的</div>
    </div>
  </div>
  <div id="toast"></div>`;
}

function doSearch(e) {
  e.preventDefault();
  const q = e.target.q.value.trim();
  if (q) location.href = `index.html?q=${encodeURIComponent(q)}`;
}
function authGo(url) {
  if (!Auth.cur()) { location.href = 'login.html'; return; }
  location.href = url;
}

// ===================== 商品卡片 =====================
function renderGoodsCard(p) {
  const emoji = catEmoji(p.category);
  return `
  <div class="goods-card" onclick="location.href='product.html?id=${p.id}'">
    ${p.images && p.images[0]
      ? `<img class="goods-card-img" src="${p.images[0]}" loading="lazy">`
      : `<div class="goods-card-img-placeholder">${emoji}</div>`}
    <div class="goods-card-body">
      <div class="goods-card-title">${escHtml(p.title)}</div>
      <div class="goods-card-price">${fmtMoney(p.price)}<span>${fmtMoney(p.origPrice)}</span></div>
      <div class="goods-card-meta">
        <span class="badge">${p.condition}</span>
        <span>${fmtDate(p.createdAt)}</span>
      </div>
    </div>
  </div>`;
}

function catEmoji(cat) {
  const map = { '手机数码': '📱', '服装服饰': '👗', '图书教材': '📚', '自行车': '🚲', '家居生活': '🏠', '运动户外': '⚽', '美妆护肤': '💄', '其他': '📦' };
  return map[cat] || '🛍️';
}

function escHtml(s) {
  return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}

// ===================== 会话辅助 =====================
function getConversations(userId) {
  const msgs = DB.getArr('messages');
  const map = {};
  msgs.forEach(m => {
    if (m.fromId !== userId && m.toId !== userId) return;
    const otherId = m.fromId === userId ? m.toId : m.fromId;
    const key = [otherId, m.productId].join(':');
    if (!map[key] || m.createdAt > map[key].lastMsg.createdAt) {
      map[key] = { otherId, productId: m.productId, lastMsg: m };
    }
  });
  return Object.values(map).sort((a, b) => b.lastMsg.createdAt - a.lastMsg.createdAt);
}

// ===================== 用户辅助 =====================
function getUserById(id) {
  return DB.getArr('users').find(u => u.id === id);
}
function getProductById(id) {
  return DB.getArr('products').find(p => p.id === id);
}

// ===================== 通知 =====================
function addNotif(userId, type, content) {
  DB.push('notifications', { id: uid(), userId, type, content, createdAt: Date.now(), read: false });
}

// ===================== 图片压缩(base64) =====================
function compressImage(file, maxW = 800) {
  return new Promise(resolve => {
    const reader = new FileReader();
    reader.onload = e => {
      const img = new Image();
      img.onload = () => {
        const canvas = document.createElement('canvas');
        let w = img.width, h = img.height;
        if (w > maxW) { h = Math.round(h * maxW / w); w = maxW; }
        canvas.width = w; canvas.height = h;
        canvas.getContext('2d').drawImage(img, 0, 0, w, h);
        resolve(canvas.toDataURL('image/jpeg', 0.8));
      };
      img.src = e.target.result;
    };
    reader.readAsDataURL(file);
  });
}
