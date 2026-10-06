(function (root) {
  const repo = 'https://github.com/Zayoooh1/WindowsProcessControlCenter';
  function parse(version) {
    const m = String(version || '').replace(/^v/i, '').match(/^(\d+)\.(\d+)\.(\d+)(?:-([0-9A-Za-z.-]+))?(?:\+[0-9A-Za-z.-]+)?$/);
    return m ? { numbers: m.slice(1, 4).map(Number), pre: m[4]?.split('.') || [] } : null;
  }
  function compare(a, b) {
    a = parse(a); b = parse(b);
    if (!a || !b) return null;
    for (let i = 0; i < 3; i++) if (a.numbers[i] !== b.numbers[i]) return a.numbers[i] < b.numbers[i] ? -1 : 1;
    if (!a.pre.length || !b.pre.length) return a.pre.length === b.pre.length ? 0 : (a.pre.length ? -1 : 1);
    for (let i = 0; i < Math.max(a.pre.length, b.pre.length); i++) {
      if (a.pre[i] === b.pre[i]) continue;
      if (a.pre[i] === undefined || b.pre[i] === undefined) return a.pre[i] === undefined ? -1 : 1;
      const an = /^\d+$/.test(a.pre[i]), bn = /^\d+$/.test(b.pre[i]);
      if (an && bn) return Number(a.pre[i]) < Number(b.pre[i]) ? -1 : 1;
      if (an !== bn) return an ? -1 : 1;
      return a.pre[i] < b.pre[i] ? -1 : 1;
    }
    return 0;
  }
  function safeUrl(value, download = false) {
    try {
      const u = new URL(value);
      const prefix = '/Zayoooh1/WindowsProcessControlCenter/releases/' + (download ? 'download/' : '');
      return u.protocol === 'https:' && u.hostname === 'github.com' && !u.username && !u.password && !u.port && u.pathname.startsWith(prefix) ? u.href : null;
    } catch { return null; }
  }
  function select(releases, channel) {
    if (!Array.isArray(releases)) throw new Error('GitHub returned an invalid release list.');
    return releases.filter(r => r && !r.draft && (channel === 'prerelease' || !r.prerelease) && parse(r.tag_name))
      .sort((a, b) => compare(b.tag_name, a.tag_name))[0] || null;
  }
  async function check({ current, channel = 'stable', fetcher = root.fetch.bind(root), signal }) {
    if (!parse(current)) throw new Error('The current application version could not be read.');
    const url = 'https://api.github.com/repos/Zayoooh1/WindowsProcessControlCenter/releases' +
      (channel === 'prerelease' ? '?per_page=100' : '/latest');
    const response = await fetcher(url, { headers: { Accept: 'application/vnd.github+json' }, signal });
    if (!response.ok) {
      if (response.status === 404) throw new Error('No public release was found for this channel (HTTP 404). Check the release page or select another channel.');
      if (response.status === 403 || response.status === 429) throw new Error('GitHub denied the request or its API rate limit was reached (HTTP ' + response.status + '). Try again later.');
      throw new Error('GitHub update check failed (HTTP ' + response.status + ').');
    }
    let data;
    try { data = await response.json(); } catch { throw new Error('GitHub returned unreadable update information.'); }
    if (channel !== 'prerelease' && (!data || typeof data !== 'object' || Array.isArray(data))) throw new Error('GitHub returned invalid release information.');
    const release = select(channel === 'prerelease' ? data : [data], channel);
    if (!release) return { available: false, release: null };
    const assets = Array.isArray(release.assets) ? release.assets : [];
    const asset = pattern => assets.find(a => pattern.test(String(a.name)) && safeUrl(a.browser_download_url, true));
    const setup = asset(/[-.]setup\.exe$/i), portable = asset(/[-.]portable\.zip$/i);
    return { available: compare(current, release.tag_name) < 0, release: {
      parsedVersion: release.tag_name.replace(/^v/i, ''), releaseName: String(release.name || release.tag_name),
      releaseUrl: safeUrl(release.html_url), body: String(release.body || ''), prerelease: Boolean(release.prerelease),
      installerExe: setup ? safeUrl(setup.browser_download_url, true) : null,
      portableZip: portable ? safeUrl(portable.browser_download_url, true) : null,
    } };
  }
  const api = { parse, compare, select, check, safeUrl };
  root.WPCCUpdates = api;
  if (typeof module !== 'undefined') module.exports = api;
})(globalThis);
