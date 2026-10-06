const assert = require('node:assert/strict');
const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');
let playwright;
try { playwright = require('playwright'); } catch { playwright = require(path.join(process.env.CODEX_PRIMARY_RUNTIME_NODE_MODULES, 'playwright')); }
const root = path.join(__dirname, '..', 'web');
const server = http.createServer((req, res) => {
  const target = path.join(root, req.url === '/' ? 'index.html' : req.url.split('?')[0]);
  if (!target.startsWith(root) || !fs.existsSync(target)) { res.writeHead(404); return res.end(); }
  res.setHeader('Content-Type', target.endsWith('.js') ? 'text/javascript' : target.endsWith('.css') ? 'text/css' : 'text/html');
  res.end(fs.readFileSync(target));
});
const repo = 'https://github.com/Zayoooh1/WindowsProcessControlCenter/releases/';
const release = (tag, prerelease = false) => ({ tag_name: tag, prerelease, name: 'WPCC '+tag, body: '- Fixed IFEO reads\n- Compact startup list', html_url: repo+'tag/'+tag, assets: [
  {name:'WPCC-Setup.exe',browser_download_url:repo+'download/'+tag+'/WPCC-Setup.exe'},
  {name:'WPCC-Portable.zip',browser_download_url:repo+'download/'+tag+'/WPCC-Portable.zip'}
] });
(async () => {
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const browser = await playwright.chromium.launch({headless:true, ...(fs.existsSync(playwright.chromium.executablePath()) ? {executablePath:playwright.chromium.executablePath()} : {})});
  try {
    const page = await browser.newPage({viewport:{width:1280,height:720}});
    const errors = []; page.on('pageerror', error => errors.push(error.message));
    await page.addInitScript(() => {
      window.sent = []; let callback;
      window.chrome = {webview:{addEventListener:(event, fn)=>{callback=fn;},postMessage:message=>{
        window.sent.push(message);
        if (message.type === 'getSettings') setTimeout(()=>callback({data:{type:'settingsLoaded',success:true,startupKnown:true,settings:{updateChecksEnabled:false}}}),0);
        if (message.type === 'saveSettings') setTimeout(()=>callback({data:{type:'settingsSaved',success:true,requestId:message.requestId,startupKnown:true,startupEnabled:false,settings:JSON.parse(message.settings)}}),0);
      }}};
      window.emit = data => callback({data});
    });
    let api = release('v0.1.15-rc.1'), status = 200, offline = false;
    await page.route('https://api.github.com/**', async route => {
      if (offline) return route.abort('internetdisconnected');
      await route.fulfill({status,contentType:'application/json',body:JSON.stringify(api)});
    });
    await page.goto('http://127.0.0.1:'+server.address().port);
    await page.locator('#settingsNavButton').click();
    const clickCheck = async () => { await page.locator('#manualCheckButton').click(); await page.waitForFunction(()=>!document.getElementById('manualCheckButton').disabled); };
    // Stable channel: a newer stable release opens the real app modal from the button.
    api = release('v0.1.16'); await clickCheck();
    await page.locator('.update-modal').waitFor();
    assert.match(await page.locator('.update-info').innerText(), /0.1.16/);
    assert.match(await page.locator('.update-notes').innerText(), /Fixed IFEO/);
    assert.equal(await page.evaluate(()=>sent.some(m=>m.type==='downloadUpdate'||m.type==='executeInstaller')),false);
    await page.getByRole('button',{name:'Download portable ZIP',exact:true}).click();
    assert(await page.evaluate(()=>sent.some(m=>m.type==='OpenExternalUrl'&&m.url.endsWith('-Portable.zip'))));
    assert.equal(await page.evaluate(()=>sent.some(m=>m.type==='downloadUpdate')),false);
    await page.getByRole('button',{name:'Download installer',exact:true}).click();
    assert(await page.evaluate(()=>sent.some(m=>m.type==='downloadUpdate'&&m.url.endsWith('-Setup.exe'))));
    await page.evaluate(()=>emit({type:'downloadComplete',success:true,filePath:'C:\\Temp\\WPCC_Update_Setup.exe'}));
    assert.equal(await page.evaluate(()=>sent.some(m=>m.type==='executeInstaller')),false);
    await page.locator('#updateCompletionActions').getByRole('button',{name:'Install now',exact:true}).click();
    assert(await page.evaluate(()=>sent.some(m=>m.type==='executeInstaller')));
    // Manual checks override the ignored release; automatic checks keep that preference.
    await page.evaluate(()=>{state.settings.ignoredUpdateVersion='0.1.16';saveUpdateState({ignoredVersion:'0.1.16'});});
    await clickCheck(); assert(await page.locator('.update-modal').isVisible());
    await page.getByRole('button',{name:'Remind me later',exact:true}).click();
    await page.evaluate(()=>checkForUpdates(false)); assert.equal(await page.locator('.update-modal').count(),0);
    api = release('v0.1.15'); await clickCheck();
    assert(await page.locator('.update-modal').isVisible()); // final 0.1.15 is newer than the running RC
    await page.getByRole('button',{name:'Remind me later',exact:true}).click();
    api = release('v0.1.14'); await clickCheck(); assert.equal(await page.locator('.update-modal').count(),0);
    assert.match(await page.locator('#updateStatusArea').innerText(), /up to date/);
    await page.selectOption('#updateChannelSelect','prerelease');
    api = [release('v0.1.14'),release('v0.1.16-rc.1',true)]; await clickCheck();
    assert.match(await page.locator('.update-info').innerText(), /Test release/);
    await page.getByRole('button',{name:'Remind me later',exact:true}).click();
    status=403; await clickCheck(); assert.match(await page.locator('#updateStatusArea').innerText(),/rate limit/);
    status=200; offline=true; await clickCheck(); assert.match(await page.locator('#updateStatusArea').innerText(),/connect to GitHub/); offline=false;
    // Render a large list with long paths and read-only Image Hijacks.
    await page.locator('#autorunsNavButton').click();
    await page.evaluate(() => emit({type:'autorunsSnapshot',warning:'Image Hijacks (32-bit view): Access denied (Windows error 5). Available results are still shown.',entries:Array.from({length:1200},(_,i)=>({
      id:'entry-'+i,entryName:'Example program '+i,publisher:i%2?'Example Company':'',category:i===0?'imageHijack':'logon',enabled:true,enabledKnown:true,canSetEnabled:i!==0,
      imagePath:'C:\\Very long directory\\'+'folder\\'.repeat(50)+'program.exe',command:'program.exe --example',location:'HKLM\\Software\\Example (64-bit view)',user:'All users',status:'OK',readOnlyReason:i===0?'Launch overrides are read-only.':''
    }))}));
    assert.equal(await page.locator('#autorunsRows > tr:not(.autorun-details-row)').count(),100);
    assert.match(await page.locator('#autorunsWarning').innerText(),/Available results/);
    assert(await page.locator('[data-autorun-id="entry-0"] input').isDisabled());
    await page.locator('[data-autorun-id="entry-0"] button').click();
    assert.match(await page.locator('.autorun-details-row').first().innerText(),/HKLM/);
    assert.match(await page.locator('.autorun-details-row').first().innerText(),/Launch overrides are read-only/);
    assert.equal(await page.locator('[data-autorun-id="entry-0"]').evaluate(el=>el.getBoundingClientRect().height),38);
    const output = process.env.WPCC_UI_OUTPUT || path.join(__dirname,'..','build','ui-results'); fs.mkdirSync(output,{recursive:true});
    await page.screenshot({path:path.join(output,'autoruns-1280.png')});
    await page.locator('#autorunsNext').click(); assert.match(await page.locator('#autorunsPageLabel').innerText(),/101 to 200/);
    await page.setViewportSize({width:900,height:600});
    await page.screenshot({path:path.join(output,'autoruns-900.png')});
    assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
    await page.locator('#autorunsSearchInput').fill('Example program 1199');
    assert.equal(await page.locator('#autorunsRows > tr:not(.autorun-details-row)').count(),1);
    await page.locator('#autorunsSearchInput').fill('');
    await page.locator('[data-autoruns-category="imageHijack"]').click();
    assert.match(await page.locator('#autorunsCategoryHelp').innerText(),/does not mean an entry is malicious/);
    assert.equal(await page.locator('#autorunsRows > tr:not(.autorun-details-row)').count(),1);
    assert.deepEqual(errors,[]);
    console.log('UI scenarios passed: manual button and modal, stable/RC/no update, ignored release, network/rate errors, Setup confirmation, Portable link, 1200 rows, pagination, long paths, read-only limits, small viewport and search/category filters. Native host and GitHub responses were mocked.');
  } finally { await browser.close(); }
})().catch(error=>{console.error(error);process.exitCode=1;}).finally(()=>server.close());
