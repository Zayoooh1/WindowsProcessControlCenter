const assert = require("node:assert/strict");
const Queue = require("../web/settings-queue.js");
const sent = [], accepted = [];
const queue = new Queue(message => sent.push(message), (message, pending) => accepted.push({message, pending}));
const reply = (id, enabled, success = true) => ({requestId:id, startupEnabled:enabled, startupKnown:true, success});
queue.enqueue({startWithWindows:true, favorites:["a"]}, true);
assert.equal(sent.length, 1);
assert.equal(sent[0].startupChange, true);
queue.enqueue({startWithWindows:false, favorites:["a"]}, true);
queue.enqueue({startWithWindows:false, favorites:["a", "b"]});
assert.equal(sent.length, 1);
assert.equal(queue.receive(reply(99, false)), false);
assert.equal(accepted.length, 0);
queue.receive(reply(1, true));
assert.equal(sent.length, 2);
assert.equal(sent[1].startupChange, true);
assert.deepEqual(JSON.parse(sent[1].settings), {startWithWindows:false, favorites:["a", "b"]});
assert.equal(accepted[0].pending.startWithWindows, false);
assert.equal(queue.receive(reply(1, true)), false); // duplicate cannot overwrite latest
queue.receive(reply(2, false));
assert.equal(queue.inFlight, null);
queue.enqueue({startWithWindows:true}, true);
queue.enqueue({startWithWindows:true, minimizeToTray:true});
queue.receive(reply(3, false, false)); // failed enable must not get retried by unrelated tray edit
assert.equal(sent[3].startupChange, false);
assert.equal(JSON.parse(sent[3].settings).startWithWindows, false);
assert.equal(JSON.parse(sent[3].settings).minimizeToTray, true);
queue.receive(reply(4, false));
queue.enqueue({startWithWindows:true}, true);
queue.enqueue({startWithWindows:false}, true);
queue.enqueue({startWithWindows:true}, true);
queue.receive(reply(5, false, false));
assert.equal(sent[5].startupChange, true);
assert.equal(JSON.parse(sent[5].settings).startWithWindows, true); // latest explicit intent survives
queue.receive(reply(6, true));
assert.equal(accepted.at(-1).pending, null);
console.log("6 settings queue scenarios passed");
