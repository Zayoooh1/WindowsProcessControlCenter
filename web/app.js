const state = {
  processes: [],
  filtered: [],
  selectedPid: null,
  query: "",
  pendingPriorityPid: null,
  pendingTerminatePid: null,
  pendingFreezePid: null,
  pendingResumePid: null,
  terminateModalProcess: null,
  freezeModalProcess: null,
  actionResult: null,
};

const elements = {
  refreshButton: document.getElementById("refreshButton"),
  processCount: document.getElementById("processCount"),
  snapshotSummary: document.getElementById("snapshotSummary"),
  searchInput: document.getElementById("searchInput"),
  processRows: document.getElementById("processRows"),
  detailsContent: document.getElementById("detailsContent"),
  errorBanner: document.getElementById("errorBanner"),
};

function postToHost(message) {
  if (!window.chrome?.webview) {
    showError("WebView2 bridge is not available.");
    return;
  }

  window.chrome.webview.postMessage(message);
}

function requestProcesses() {
  hideError();
  elements.refreshButton.disabled = true;
  postToHost({ type: "refreshProcesses" });
}

function handleHostMessage(event) {
  const message = event.data;
  if (!message || typeof message.type !== "string") {
    showError("Received an invalid backend message.");
    return;
  }

  if (message.type === "processSnapshot") {
    elements.refreshButton.disabled = false;
    state.processes = Array.isArray(message.processes) ? message.processes : [];
    if (!state.processes.some((process) => process.pid === state.selectedPid)) {
      state.selectedPid = state.processes[0]?.pid ?? null;
    }
    applyFilter();
    return;
  }

  if (message.type === "actionResult" && message.action === "setCpuPriority") {
    elements.refreshButton.disabled = false;
    state.pendingPriorityPid = null;
    state.actionResult = message;
    render();
    return;
  }

  if (message.type === "actionResult" && message.action === "terminateProcess") {
    elements.refreshButton.disabled = false;
    state.pendingTerminatePid = null;
    state.terminateModalProcess = null;
    state.actionResult = message;
    showStatus(message.message || "End process action completed.", Boolean(message.success));
    render();
    return;
  }

  if (message.type === "actionResult" && (message.action === "freezeProcess" || message.action === "resumeProcess")) {
    elements.refreshButton.disabled = false;
    state.pendingFreezePid = null;
    state.pendingResumePid = null;
    state.freezeModalProcess = null;
    state.actionResult = message;
    showStatus(message.message || "Process runtime action completed.", Boolean(message.success));
    render();
    return;
  }

  if (message.type === "error") {
    elements.refreshButton.disabled = false;
    showError(message.message || "Unknown backend error.");
  }
}

function applyFilter() {
  const query = state.query.trim().toLowerCase();
  state.filtered = query
    ? state.processes.filter((process) => {
        const pid = String(process.pid ?? "");
        const name = String(process.name ?? "").toLowerCase();
        const path = String(process.path ?? "").toLowerCase();
        return pid.includes(query) || name.includes(query) || path.includes(query);
      })
    : [...state.processes];

  if (!state.filtered.some((process) => process.pid === state.selectedPid)) {
    state.selectedPid = state.filtered[0]?.pid ?? null;
  }

  render();
}

function render() {
  elements.processCount.textContent = `${state.processes.length} processes`;
  elements.snapshotSummary.textContent = `${state.filtered.length} shown from ${state.processes.length} active processes`;
  renderRows();
  renderDetails();
  renderTerminateModal();
  renderFreezeModal();
}

function renderRows() {
  elements.processRows.replaceChildren();

  if (state.filtered.length === 0) {
    const row = document.createElement("tr");
    const cell = document.createElement("td");
    cell.colSpan = 7;
    cell.className = "empty-cell";
    cell.textContent = state.processes.length === 0 ? "No process snapshot loaded." : "No processes match the current search.";
    row.appendChild(cell);
    elements.processRows.appendChild(row);
    return;
  }

  for (const process of state.filtered) {
    const row = document.createElement("tr");
    row.className = process.pid === state.selectedPid ? "selected" : "";
    row.addEventListener("click", () => {
      state.selectedPid = process.pid;
      render();
    });

    row.appendChild(textCell(process.pid));
    row.appendChild(textCell(process.name || "Unknown"));
    row.appendChild(pathCell(process.path || "Unavailable"));
    row.appendChild(badgeCell(runtimeLabel(process), runtimeTone(process)));
    row.appendChild(badgeCell(process.cpuPriority || "Unknown", priorityTone(process.cpuPriority)));
    row.appendChild(badgeCell(process.adminNeeded ? "Likely" : "No", process.adminNeeded ? "warning" : "neutral"));
    row.appendChild(badgeCell(process.accessStatus || "Unknown", accessTone(process.accessStatus)));
    elements.processRows.appendChild(row);
  }
}

function renderDetails() {
  const selected = state.processes.find((process) => process.pid === state.selectedPid);
  elements.detailsContent.replaceChildren();

  if (!selected) {
    elements.detailsContent.className = "details-content empty-details";
    elements.detailsContent.textContent = "Select a process to inspect its details.";
    return;
  }

  elements.detailsContent.className = "details-content";
  elements.detailsContent.appendChild(section("Basic information", [
    valueLine(selected.name || "Unknown"),
    mutedLine(`PID ${selected.pid}`),
  ]));
  elements.detailsContent.appendChild(section("Executable path", [
    valueLine(selected.path || "Unavailable", selected.path || "Unavailable"),
  ]));
  elements.detailsContent.appendChild(section("Runtime status", [
    badge(runtimeLabel(selected), runtimeTone(selected)),
  ]));
  elements.detailsContent.appendChild(cpuPrioritySection(selected));
  elements.detailsContent.appendChild(section("Access status", [
    badge(selected.accessStatus || "Unknown", accessTone(selected.accessStatus)),
  ]));
  elements.detailsContent.appendChild(section("Admin requirement", [
    valueLine(selected.adminNeeded ? "Likely required for some future process actions" : "Not detected for this read-only snapshot"),
  ]));
  elements.detailsContent.appendChild(freezeResumeSection(selected));
  elements.detailsContent.appendChild(terminateSection(selected));

  if (selected.accessError) {
    elements.detailsContent.appendChild(section("Access error", [
      valueLine(selected.accessError, selected.accessError),
    ]));
  }

  const actions = document.createElement("div");
  actions.className = "future-actions";
  for (const label of ["Set GPU Preference"]) {
    const button = document.createElement("button");
    button.type = "button";
    button.disabled = true;
    button.textContent = label;
    button.title = "Not implemented yet";
    actions.appendChild(button);
  }
  elements.detailsContent.appendChild(section("Future actions", [actions]));
}

function cpuPrioritySection(process) {
  const container = document.createElement("section");
  container.className = "detail-section cpu-priority-section";

  const heading = document.createElement("div");
  heading.className = "section-label";
  heading.textContent = "CPU priority";
  container.appendChild(heading);
  container.appendChild(badge(process.cpuPriority || "Unknown", priorityTone(process.cpuPriority)));

  const unavailableReason = getPriorityUnavailableReason(process);
  const form = document.createElement("div");
  form.className = "priority-form";

  const select = document.createElement("select");
  select.className = "priority-select";
  const priorities = ["Realtime", "High", "Above Normal", "Normal", "Below Normal", "Idle"];
  const currentPriority = normalizePriority(process.cpuPriority);
  for (const priority of priorities) {
    const option = document.createElement("option");
    option.value = priority;
    option.textContent = priority;
    option.selected = priority === currentPriority;
    select.appendChild(option);
  }

  const realtimeWarning = document.createElement("div");
  realtimeWarning.className = "realtime-warning hidden";
  realtimeWarning.textContent = "Realtime priority can make Windows less responsive. Use it only when you understand the risk.";

  const confirmLabel = document.createElement("label");
  confirmLabel.className = "confirm-realtime hidden";
  const confirmCheckbox = document.createElement("input");
  confirmCheckbox.type = "checkbox";
  confirmLabel.appendChild(confirmCheckbox);
  confirmLabel.appendChild(document.createTextNode("I understand the risk of Realtime priority"));

  const applyButton = document.createElement("button");
  applyButton.className = "apply-priority-button";
  applyButton.type = "button";
  applyButton.textContent = state.pendingPriorityPid === process.pid ? "Applying..." : "Apply Priority";

  const updateRealtimeUi = () => {
    const realtime = select.value === "Realtime";
    realtimeWarning.classList.toggle("hidden", !realtime);
    confirmLabel.classList.toggle("hidden", !realtime);
    applyButton.disabled = Boolean(unavailableReason) || state.pendingPriorityPid === process.pid || (realtime && !confirmCheckbox.checked);
  };

  select.addEventListener("change", () => {
    state.actionResult = null;
    updateRealtimeUi();
    renderActionResult(container, process.pid);
  });
  confirmCheckbox.addEventListener("change", updateRealtimeUi);
  applyButton.addEventListener("click", () => {
    if (select.value === "Realtime" && !confirmCheckbox.checked) {
      return;
    }

    state.pendingPriorityPid = process.pid;
    state.actionResult = null;
    render();
    postToHost({
      type: "setCpuPriority",
      pid: process.pid,
      priority: select.value,
      confirmRealtime: select.value === "Realtime" && confirmCheckbox.checked,
    });
  });

  if (unavailableReason) {
    const disabledReason = document.createElement("div");
    disabledReason.className = "priority-disabled-reason";
    disabledReason.textContent = unavailableReason;
    form.appendChild(disabledReason);
    select.disabled = true;
  }

  form.appendChild(select);
  form.appendChild(realtimeWarning);
  form.appendChild(confirmLabel);
  form.appendChild(applyButton);
  container.appendChild(form);
  renderActionResult(container, process.pid);
  updateRealtimeUi();
  return container;
}

function terminateSection(process) {
  const unavailableReason = getTerminateUnavailableReason(process);
  const container = document.createElement("section");
  container.className = "detail-section terminate-section";

  const heading = document.createElement("div");
  heading.className = "section-label";
  heading.textContent = "End process";
  container.appendChild(heading);

  const note = document.createElement("div");
  note.className = "terminate-note";
  note.textContent = "Terminates a single selected process after explicit confirmation.";
  container.appendChild(note);

  if (unavailableReason) {
    const reason = document.createElement("div");
    reason.className = "priority-disabled-reason";
    reason.textContent = unavailableReason;
    container.appendChild(reason);
  }

  const button = document.createElement("button");
  button.type = "button";
  button.className = "danger-action-button";
  button.textContent = state.pendingTerminatePid === process.pid ? "Ending..." : "End Process";
  button.disabled = Boolean(unavailableReason) || state.pendingTerminatePid === process.pid;
  button.addEventListener("click", () => {
    state.actionResult = null;
    state.terminateModalProcess = process;
    render();
  });
  container.appendChild(button);
  renderActionResult(container, process.pid, "terminateProcess");
  return container;
}

function freezeResumeSection(process) {
  const container = document.createElement("section");
  container.className = "detail-section process-control-section";

  const heading = document.createElement("div");
  heading.className = "section-label";
  heading.textContent = "Freeze / Resume";
  container.appendChild(heading);

  const note = document.createElement("div");
  note.className = "terminate-note";
  note.textContent = "Freeze suspends this process' current threads. Resume only restores threads frozen by this app.";
  container.appendChild(note);

  const freezeReason = getFreezeUnavailableReason(process);
  if (freezeReason) {
    const reason = document.createElement("div");
    reason.className = "priority-disabled-reason";
    reason.textContent = freezeReason;
    container.appendChild(reason);
  }

  const resumeReason = getResumeUnavailableReason(process);
  if (resumeReason) {
    const reason = document.createElement("div");
    reason.className = "priority-disabled-reason neutral-reason";
    reason.textContent = resumeReason;
    container.appendChild(reason);
  }

  const actions = document.createElement("div");
  actions.className = "control-actions";

  const freezeButton = document.createElement("button");
  freezeButton.type = "button";
  freezeButton.className = "warning-action-button";
  freezeButton.textContent = state.pendingFreezePid === process.pid ? "Freezing..." : "Freeze";
  freezeButton.disabled = Boolean(freezeReason) || state.pendingFreezePid === process.pid;
  freezeButton.addEventListener("click", () => {
    state.actionResult = null;
    state.freezeModalProcess = process;
    render();
  });

  const resumeButton = document.createElement("button");
  resumeButton.type = "button";
  resumeButton.className = "secondary-button";
  resumeButton.textContent = state.pendingResumePid === process.pid ? "Resuming..." : "Resume";
  resumeButton.disabled = Boolean(resumeReason) || state.pendingResumePid === process.pid;
  resumeButton.addEventListener("click", () => {
    state.pendingResumePid = process.pid;
    state.actionResult = null;
    render();
    postToHost({
      type: "resumeProcess",
      pid: process.pid,
      expectedName: process.name || "",
    });
  });

  actions.appendChild(freezeButton);
  actions.appendChild(resumeButton);
  container.appendChild(actions);
  if (state.actionResult?.action === "resumeProcess") {
    renderActionResult(container, process.pid, "resumeProcess");
  } else {
    renderActionResult(container, process.pid, "freezeProcess");
  }
  return container;
}

function renderTerminateModal() {
  document.querySelector(".modal-backdrop")?.remove();

  const process = state.terminateModalProcess;
  if (!process) {
    return;
  }

  const expectedText = getTerminateConfirmationText(process);
  const backdrop = document.createElement("div");
  backdrop.className = "modal-backdrop";

  const modal = document.createElement("div");
  modal.className = "confirm-modal";
  modal.setAttribute("role", "dialog");
  modal.setAttribute("aria-modal", "true");
  modal.setAttribute("aria-label", "Confirm end process");

  const title = document.createElement("h2");
  title.textContent = "End process?";
  modal.appendChild(title);

  const warning = document.createElement("div");
  warning.className = "modal-warning";
  warning.textContent = "Unsaved data in this process may be lost. This action targets only the selected process, not its child processes.";
  modal.appendChild(warning);

  modal.appendChild(modalInfoLine("Process", process.name || "Unknown"));
  modal.appendChild(modalInfoLine("PID", String(process.pid)));
  modal.appendChild(modalInfoLine("Executable path", process.path || "Unavailable", process.path || ""));

  const label = document.createElement("label");
  label.className = "confirmation-field";
  const helper = document.createElement("span");
  helper.textContent = `Type ${expectedText} to confirm`;
  const input = document.createElement("input");
  input.type = "text";
  input.autocomplete = "off";
  input.spellcheck = false;
  label.appendChild(helper);
  label.appendChild(input);
  modal.appendChild(label);

  const actions = document.createElement("div");
  actions.className = "modal-actions";

  const cancelButton = document.createElement("button");
  cancelButton.type = "button";
  cancelButton.className = "secondary-button";
  cancelButton.textContent = "Cancel";
  cancelButton.addEventListener("click", () => {
    state.terminateModalProcess = null;
    render();
  });

  const confirmButton = document.createElement("button");
  confirmButton.type = "button";
  confirmButton.className = "danger-action-button";
  confirmButton.textContent = state.pendingTerminatePid === process.pid ? "Ending..." : "Confirm End Process";
  confirmButton.disabled = true;

  const updateConfirmState = () => {
    confirmButton.disabled = state.pendingTerminatePid === process.pid || !terminateConfirmationMatches(process, input.value);
  };

  input.addEventListener("input", updateConfirmState);
  confirmButton.addEventListener("click", () => {
    if (!terminateConfirmationMatches(process, input.value)) {
      return;
    }

    state.pendingTerminatePid = process.pid;
    state.actionResult = null;
    renderTerminateModal();
    postToHost({
      type: "terminateProcess",
      pid: process.pid,
      expectedName: process.name || "",
      confirmation: input.value.trim(),
    });
  });

  actions.appendChild(cancelButton);
  actions.appendChild(confirmButton);
  modal.appendChild(actions);
  backdrop.appendChild(modal);
  document.body.appendChild(backdrop);
  input.focus();
}

function renderFreezeModal() {
  document.querySelector(".freeze-modal-backdrop")?.remove();

  const process = state.freezeModalProcess;
  if (!process) {
    return;
  }

  const expectedText = getProcessConfirmationText(process);
  const backdrop = document.createElement("div");
  backdrop.className = "modal-backdrop freeze-modal-backdrop";

  const modal = document.createElement("div");
  modal.className = "confirm-modal";
  modal.setAttribute("role", "dialog");
  modal.setAttribute("aria-modal", "true");
  modal.setAttribute("aria-label", "Confirm freeze process");

  const title = document.createElement("h2");
  title.textContent = "Freeze process?";
  modal.appendChild(title);

  const warning = document.createElement("div");
  warning.className = "modal-warning";
  warning.textContent = "Freezing can stop this app's windows and work until you resume it. WPCC will also try to resume processes it froze when closing.";
  modal.appendChild(warning);

  modal.appendChild(modalInfoLine("Process", process.name || "Unknown"));
  modal.appendChild(modalInfoLine("PID", String(process.pid)));
  modal.appendChild(modalInfoLine("Executable path", process.path || "Unavailable", process.path || ""));

  const label = document.createElement("label");
  label.className = "confirmation-field";
  const helper = document.createElement("span");
  helper.textContent = `Type ${expectedText} to confirm`;
  const input = document.createElement("input");
  input.type = "text";
  input.autocomplete = "off";
  input.spellcheck = false;
  label.appendChild(helper);
  label.appendChild(input);
  modal.appendChild(label);

  const actions = document.createElement("div");
  actions.className = "modal-actions";

  const cancelButton = document.createElement("button");
  cancelButton.type = "button";
  cancelButton.className = "secondary-button";
  cancelButton.textContent = "Cancel";
  cancelButton.addEventListener("click", () => {
    state.freezeModalProcess = null;
    render();
  });

  const confirmButton = document.createElement("button");
  confirmButton.type = "button";
  confirmButton.className = "warning-action-button";
  confirmButton.textContent = state.pendingFreezePid === process.pid ? "Freezing..." : "Confirm Freeze";
  confirmButton.disabled = true;

  const updateConfirmState = () => {
    confirmButton.disabled = state.pendingFreezePid === process.pid || !processConfirmationMatches(process, input.value);
  };

  input.addEventListener("input", updateConfirmState);
  confirmButton.addEventListener("click", () => {
    if (!processConfirmationMatches(process, input.value)) {
      return;
    }

    state.pendingFreezePid = process.pid;
    state.actionResult = null;
    renderFreezeModal();
    postToHost({
      type: "freezeProcess",
      pid: process.pid,
      expectedName: process.name || "",
      confirmation: input.value.trim(),
    });
  });

  actions.appendChild(cancelButton);
  actions.appendChild(confirmButton);
  modal.appendChild(actions);
  backdrop.appendChild(modal);
  document.body.appendChild(backdrop);
  input.focus();
}

function modalInfoLine(label, value, title) {
  const row = document.createElement("div");
  row.className = "modal-info-line";
  const labelElement = document.createElement("span");
  labelElement.textContent = label;
  const valueElement = document.createElement("strong");
  valueElement.textContent = value;
  if (title) {
    valueElement.title = title;
  }
  row.appendChild(labelElement);
  row.appendChild(valueElement);
  return row;
}

function renderActionResult(container, pid, action = "setCpuPriority") {
  const existing = container.querySelector(".action-message");
  existing?.remove();

  if (!state.actionResult || state.actionResult.pid !== pid || state.actionResult.action !== action) {
    return;
  }

  const message = document.createElement("div");
  message.className = `action-message ${state.actionResult.success ? "success" : "error"}`;
  message.textContent = state.actionResult.message || (state.actionResult.success ? "Action completed." : "Action failed.");
  container.appendChild(message);
}

function getPriorityUnavailableReason(process) {
  if (process.pid === 0 || process.name === "System" || process.accessStatus === "Protected/System") {
    return "Protected/system process cannot be modified.";
  }

  if (process.accessStatus === "Access denied") {
    return "This process is not accessible. Administrator permissions may be required.";
  }

  if (process.accessStatus !== "Accessible") {
    return "This process is not accessible.";
  }

  return "";
}

function getTerminateUnavailableReason(process) {
  const name = String(process.name || "");
  const loweredName = name.toLowerCase();
  const criticalNames = new Set([
    "system",
    "registry",
    "smss.exe",
    "csrss.exe",
    "wininit.exe",
    "winlogon.exe",
    "services.exe",
    "lsass.exe",
    "svchost.exe",
    "fontdrvhost.exe",
    "dwm.exe",
    "explorer.exe",
    "audiodg.exe",
  ]);

  if (process.pid === 0 || process.pid === 4 || process.accessStatus === "Protected/System") {
    return "Protected/system process cannot be ended.";
  }

  if (loweredName === "windowsprocesscontrolcenter.exe") {
    return "Cannot end this application from itself.";
  }

  if (criticalNames.has(loweredName)) {
    return "This is a critical Windows process.";
  }

  if (process.accessStatus === "Access denied") {
    return "Access denied.";
  }

  if (process.accessStatus !== "Accessible") {
    return "This process is not accessible.";
  }

  return "";
}

function getFreezeUnavailableReason(process) {
  const baseReason = getRuntimeControlUnavailableReason(process, "frozen", "freeze");
  if (baseReason) {
    return baseReason;
  }

  if (process.isFrozenByApp) {
    return "This process is already frozen by this app.";
  }

  return "";
}

function getResumeUnavailableReason(process) {
  const baseReason = getRuntimeControlUnavailableReason(process, "resumed", "resume");
  if (baseReason) {
    return baseReason;
  }

  if (!process.isFrozenByApp) {
    return "This process was not frozen by this app.";
  }

  return "";
}

function getRuntimeControlUnavailableReason(process, passiveVerb, activeVerb) {
  const name = String(process.name || "");
  const loweredName = name.toLowerCase();
  const criticalNames = new Set([
    "system",
    "registry",
    "smss.exe",
    "csrss.exe",
    "wininit.exe",
    "winlogon.exe",
    "services.exe",
    "lsass.exe",
    "svchost.exe",
    "fontdrvhost.exe",
    "dwm.exe",
    "explorer.exe",
    "audiodg.exe",
  ]);

  if (process.pid === 0 || process.pid === 4 || process.accessStatus === "Protected/System") {
    return `Protected/system process cannot be ${passiveVerb}.`;
  }

  if (loweredName === "windowsprocesscontrolcenter.exe") {
    return `Cannot ${activeVerb} this application from itself.`;
  }

  if (criticalNames.has(loweredName)) {
    return "This is a critical Windows process.";
  }

  if (process.accessStatus === "Access denied") {
    return "Access denied.";
  }

  if (process.accessStatus !== "Accessible") {
    return "This process is not accessible.";
  }

  return "";
}

function getTerminateConfirmationText(process) {
  return getProcessConfirmationText(process);
}

function getProcessConfirmationText(process) {
  const name = String(process.name || "").trim();
  return name && name.toLowerCase() !== "unknown" ? name : String(process.pid);
}

function terminateConfirmationMatches(process, value) {
  return processConfirmationMatches(process, value);
}

function processConfirmationMatches(process, value) {
  return value.trim().toLowerCase() === getProcessConfirmationText(process).toLowerCase();
}

function normalizePriority(priority) {
  if (priority === "Above normal") {
    return "Above Normal";
  }
  if (priority === "Below normal") {
    return "Below Normal";
  }
  return priority || "Normal";
}

function textCell(value) {
  const cell = document.createElement("td");
  cell.textContent = value ?? "";
  return cell;
}

function pathCell(value) {
  const cell = document.createElement("td");
  cell.className = "path-cell";
  cell.textContent = value;
  cell.title = value;
  return cell;
}

function badgeCell(label, tone) {
  const cell = document.createElement("td");
  cell.appendChild(badge(label, tone));
  return cell;
}

function badge(label, tone = "neutral") {
  const span = document.createElement("span");
  span.className = `badge ${tone}`;
  span.textContent = label;
  return span;
}

function section(label, children) {
  const container = document.createElement("section");
  container.className = "detail-section";
  const heading = document.createElement("div");
  heading.className = "section-label";
  heading.textContent = label;
  container.appendChild(heading);
  for (const child of children) {
    container.appendChild(child);
  }
  return container;
}

function valueLine(value, title) {
  const div = document.createElement("div");
  div.className = "detail-value";
  div.textContent = value;
  if (title) {
    div.title = title;
  }
  return div;
}

function mutedLine(value) {
  const div = valueLine(value);
  div.style.color = "var(--muted)";
  return div;
}

function priorityTone(priority) {
  if (priority === "High" || priority === "Realtime") {
    return "danger";
  }
  if (priority === "Above normal") {
    return "warning";
  }
  return "neutral";
}

function accessTone(status) {
  if (status === "Accessible") {
    return "success";
  }
  if (status === "Access denied") {
    return "warning";
  }
  if (status === "Protected/System") {
    return "danger";
  }
  return "neutral";
}

function runtimeLabel(process) {
  return process.isFrozenByApp ? "Frozen by app" : "Running";
}

function runtimeTone(process) {
  return process.isFrozenByApp ? "warning" : "success";
}

function showError(message) {
  elements.errorBanner.textContent = message;
  elements.errorBanner.className = "error-banner error";
}

function showStatus(message, success) {
  elements.errorBanner.textContent = message;
  elements.errorBanner.className = `error-banner ${success ? "success" : "error"}`;
}

function hideError() {
  elements.errorBanner.classList.add("hidden");
  elements.errorBanner.classList.remove("success", "error");
  elements.errorBanner.textContent = "";
}

document.addEventListener("keydown", (event) => {
  if (event.key === "Escape" && (state.terminateModalProcess || state.freezeModalProcess)) {
    state.terminateModalProcess = null;
    state.freezeModalProcess = null;
    render();
  }
});

window.chrome?.webview?.addEventListener("message", handleHostMessage);
elements.refreshButton.addEventListener("click", requestProcesses);
elements.searchInput.addEventListener("input", (event) => {
  state.query = event.target.value;
  applyFilter();
});

requestProcesses();
