/* One native write at a time. Coalesce later edits, preserve explicit startup intent,
   and ignore late/duplicate replies rather than overwriting a newer draft. */
(function (root) {
  class SettingsQueue {
    constructor(send, accept) {
      this.send = send;
      this.accept = accept;
      this.sequence = 0;
      this.inFlight = null;
      this.pending = null;
    }
    enqueue(settings, startupChange = false) {
      this.pending = {
        settings: JSON.parse(JSON.stringify(settings)),
        startupChange: Boolean(startupChange || this.pending?.startupChange),
      };
      this.flush();
    }
    flush() {
      if (this.inFlight || !this.pending) return;
      this.inFlight = { ...this.pending, requestId: ++this.sequence };
      this.pending = null;
      this.send({ type: "saveSettings", ...this.inFlight, settings: JSON.stringify(this.inFlight.settings) });
    }
    receive(message) {
      if (!this.inFlight || message.requestId !== this.inFlight.requestId) return false;
      this.inFlight = null;
      if (this.pending && !this.pending.startupChange) {
        this.pending.settings.startWithWindows = Boolean(message.startupEnabled);
      }
      this.accept(message, this.pending?.settings ?? null);
      this.flush();
      return true;
    }
  }
  root.WPCCSettingsQueue = SettingsQueue;
  if (typeof module !== "undefined") module.exports = SettingsQueue;
})(globalThis);
