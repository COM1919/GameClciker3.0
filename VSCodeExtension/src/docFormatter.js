'use strict';
/**
 * 将 API 数据格式化为 VS 2026 风格的 Markdown 文档（悬停与补全共用）
 */

const api = require('../data/api');

const CODE_FENCE = '```';

/** 生成签名行（函数名 + 参数，可选参数用 [ ] 包裹） */
function signatureOf(def) {
  return def.signature;
}

/** 生成参数列表的 markdown */
function paramsMarkdown(def) {
  if (!def.params || def.params.length === 0) return '无参数';
  const lines = def.params.map(p => {
    const opt = p.optional ? '（可选）' : '';
    return `- \`${p.name}\`: \`${p.type}\`${opt} — ${p.desc || ''}`;
  });
  return lines.join('\n');
}

/**
 * 生成完整的 Markdown 文档
 * @param {object} def
 * @param {{includeExample?:boolean}} [opts]
 */
function formatDoc(def, opts) {
  const o = Object.assign({ includeExample: true }, opts || {});
  const parts = [];
  parts.push(`${CODE_FENCE}lua\n${signatureOf(def)}\n${CODE_FENCE}`);
  parts.push(def.description || '');
  parts.push(`**参数**\n\n${paramsMarkdown(def)}`);
  if (def.returns) parts.push(`**返回值**\n\n${def.returns}`);
  if (def.restricted) parts.push('> ⚠️ **受限 API**：需要脚本审核授权后才可使用。');
  if (def.example && o.includeExample) {
    parts.push(`**示例**\n\n${CODE_FENCE}lua\n${def.example}\n${CODE_FENCE}`);
  }
  return parts.join('\n\n');
}

/** 生成简短的纯文本签名（用于 detail / signature help 摘要） */
function detailOf(def) {
  return def.signature;
}

/** 生成某个参数在签名中的位置信息（用于 SignatureHelp 参数高亮） */
function signatureInfo(def) {
  const label = def.signature;
  const parameters = (def.params || []).map(p => {
    const optional = p.optional ? ` (可选)` : '';
    return { label: `${p.name}${optional}`, documentation: `\`${p.type}\` — ${p.desc || ''}` };
  });
  return { label, documentation: def.description || '', parameters };
}

/** 合并重复名称（例如 GetScreenSize 在功能性/GUI 两处注册） */
function dedupe(fns) {
  const seen = new Map();
  for (const f of fns) {
    if (!seen.has(f.name)) seen.set(f.name, f);
  }
  return [...seen.values()];
}

/** 按名称查找 API */
function findByName(name) {
  return api.GC_FUNCTIONS.find(f => f.name === name);
}

module.exports = {
  formatDoc,
  detailOf,
  signatureInfo,
  signatureOf,
  dedupe,
  findByName
};