const assert = require('node:assert/strict');
const service = require('../web/update-service');
const base = 'https://github.com/Zayoooh1/WindowsProcessControlCenter/releases/';
const release = (tag, prerelease = false) => ({ tag_name: tag, prerelease, html_url: base+'tag/'+tag, body: '- Fixed startup', assets: [
  { name: 'unrelated.exe', browser_download_url: base+'download/'+tag+'/unrelated.exe' },
  { name: 'WindowsProcessControlCenter-v0.1.16-Setup.exe', browser_download_url: base+'download/'+tag+'/WindowsProcessControlCenter-v0.1.16-Setup.exe' },
  { name: 'WindowsProcessControlCenter-v0.1.16-Portable.zip', browser_download_url: base+'download/'+tag+'/WindowsProcessControlCenter-v0.1.16-Portable.zip' },
] });
const fetcher = data => async () => ({ ok: true, json: async () => data });
(async () => {
  assert.equal(service.compare('0.1.14', '0.1.15-rc.1'), -1);
  assert.equal(service.compare('0.1.15-rc.1', '0.1.15-rc.2'), -1);
  assert.equal(service.compare('0.1.15-rc.2', '0.1.15'), -1);
  assert.equal(service.compare('0.1.15-rc.2', '0.1.15-rc.10'), -1);
  assert.equal((await service.check({ current:'0.1.15', fetcher:fetcher(release('v0.1.15')) })).available, false);
  const stable = await service.check({ current:'0.1.15', fetcher:fetcher(release('v0.1.16')) });
  assert(stable.available && stable.release.installerExe.endsWith('-Setup.exe') && stable.release.portableZip.endsWith('-Portable.zip'));
  assert.equal(service.select([release('v0.1.16-rc.1',true),release('v0.1.15')], 'stable').tag_name, 'v0.1.15');
  const test = await service.check({ current:'0.1.15',channel:'prerelease',fetcher:fetcher([release('v0.1.15'),release('v0.1.16-rc.1',true)]) });
  assert(test.available && test.release.prerelease);
  assert.equal((await service.check({ current:'0.1.15',channel:'prerelease',fetcher:fetcher([]) })).release, null);
  assert.equal(service.select([{...release('v9.0.0'),draft:true},release('v0.1.15')],'prerelease').tag_name,'v0.1.15');
  await assert.rejects(service.check({current:'0.1.15',fetcher:async()=>({ok:false,status:403})}), /rate limit/);
  await assert.rejects(service.check({current:'0.1.15',fetcher:async()=>({ok:false,status:404})}), /404/);
  await assert.rejects(service.check({current:'0.1.15',fetcher:async()=>({ok:false,status:500})}), /500/);
  await assert.rejects(service.check({current:'0.1.15',fetcher:async()=>({ok:true,json:async()=>{throw Error();}})}), /unreadable/);
  await assert.rejects(service.check({current:'0.1.15',fetcher:fetcher('bad')}), /invalid/);
  await assert.rejects(service.check({current:'0.1.15',fetcher:async()=>{throw new TypeError('offline');}}), /offline/);
  assert.equal(service.safeUrl('https://github.com.evil.test/asset'), null);
  assert.equal(service.safeUrl('javascript:alert(1)'),null);
  console.log('Update service scenarios passed: no update, stable, prerelease channels, RC ordering, drafts, empty results, assets, HTTP and malformed/network errors');
})();
