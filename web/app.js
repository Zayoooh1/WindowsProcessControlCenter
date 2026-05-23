const state = {
  activeView: "dashboard",
  processes: [],
  filtered: [],
  selectedPid: null,
  query: "",
  pendingPriorityPid: null,
  pendingTerminatePid: null,
  pendingFreezePid: null,
  pendingResumePid: null,
  pendingGpuPid: null,
  terminateModalProcess: null,
  freezeModalProcess: null,
  actionResult: null,
};

const elements = {
  dashboardNavButton: document.getElementById("dashboardNavButton"),
  processesNavButton: document.getElementById("processesNavButton"),
  dashboardView: document.getElementById("dashboardView"),
  processesView: document.getElementById("processesView"),
  refreshButton: document.getElementById("refreshButton"),
  dashboardRefreshButton: document.getElementById("dashboardRefreshButton"),
  goToProcessesButton: document.getElementById("goToProcessesButton"),
  quickRefreshButton: document.getElementById("quickRefreshButton"),
  quickProcessesButton: document.getElementById("quickProcessesButton"),
  processCount: document.getElementById("processCount"),
  snapshotSummary: document.getElementById("snapshotSummary"),
  dashboardSummary: document.getElementById("dashboardSummary"),
  dashboardStats: document.getElementById("dashboardStats"),
  lastActionContent: document.getElementById("lastActionContent"),
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
  elements.dashboardRefreshButton.disabled = true;
  elements.quickRefreshButton.disabled = true;
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
    elements.dashboardRefreshButton.disabled = false;
    elements.quickRefreshButton.disabled = false;
    state.processes = Array.isArray(message.processes) ? message.processes : [];
    if (!state.processes.some((process) => process.pid === state.selectedPid)) {
      state.selectedPid = state.processes[0]?.pid ?? null;
    }
    applyFilter();
    return;
  }

  if (message.type === "actionResult" && message.action === "setCpuPriority") {
    elements.refreshButton.disabled = false;
    elements.dashboardRefreshButton.disabled = false;
    elements.quickRefreshButton.disabled = false;
    state.pendingPriorityPid = null;
    state.actionResult = message;
    render();
    return;
  }

  if (message.type === "actionResult" && message.action === "terminateProcess") {
    elements.refreshButton.disabled = false;
    elements.dashboardRefreshButton.disabled = false;
    elements.quickRefreshButton.disabled = false;
    state.pendingTerminatePid = null;
    state.terminateModalProcess = null;
    state.actionResult = message;
    showStatus(message.message || "End process action completed.", Boolean(message.success));
    render();
    return;
  }

  if (message.type === "actionResult" && (message.action === "freezeProcess" || message.action === "resumeProcess")) {
    elements.refreshButton.disabled = false;
    elements.dashboardRefreshButton.disabled = false;
    elements.quickRefreshButton.disabled = false;
    state.pendingFreezePid = null;
    state.pendingResumePid = null;
    state.freezeModalProcess = null;
    state.actionResult = message;
    showStatus(message.message || "Process runtime action completed.", Boolean(message.success));
    render();
    return;
  }

  if (message.type === "actionResult" && message.action === "setGpuPreference") {
    elements.refreshButton.disabled = false;
    elements.dashboardRefreshButton.disabled = false;
    elements.quickRefreshButton.disabled = false;
    state.pendingGpuPid = null;
    state.actionResult = message;
    showStatus(message.message || "GPU preference action completed.", Boolean(message.success));
    render();
    return;
  }

  if (message.type === "error") {
    elements.refreshButton.disabled = false;
    elements.dashboardRefreshButton.disabled = false;
    elements.quickRefreshButton.disabled = false;
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
  renderActiveView();
  renderDashboard();
  renderRows();
  renderDetails();
  renderTerminateModal();
  renderFreezeModal();
}

function setActiveView(view) {
  state.activeView = view;
  render();
}

function renderActiveView() {
  const dashboardActive = state.activeView === "dashboard";
  elements.dashboardView.classList.toggle("hidden-view", !dashboardActive);
  elements.processesView.classList.toggle("hidden-view", dashboardActive);
  elements.dashboardNavButton.classList.toggle("active", dashboardActive);
  elements.processesNavButton.classList.toggle("active", !dashboardActive);
}

function renderDashboard() {
  const stats = getDashboardStats();
  elements.dashboardSummary.textContent = state.processes.length === 0
    ? "Waiting for the first process snapshot."
    : `${stats.total} processes in the current snapshot, ${stats.accessible} accessible for guarded controls.`;

  elements.dashboardStats.replaceChildren(
    statCard("Total processes", stats.total, "All processes in the latest snapshot"),
    statCard("Accessible", stats.accessible, "Processes reporting accessible status", "success"),
    statCard("Protected / denied", stats.protectedDenied, "Protected, denied, or otherwise unavailable", "warning"),
    statCard("Frozen by app", stats.frozenByApp, "Processes suspended by this WPCC session", "warning"),
    statCard("Non-normal priority", stats.nonNormalPriority, "Priority differs from Normal and is known", "neutral"),
    statCard("GPU preferences", stats.gpuPreferences, "Per-app GPU preference differs from system default", "neutral"),
  );

  renderLastAction();
}

function getDashboardStats() {
  return state.processes.reduce((stats, process) => {
    const accessStatus = String(process.accessStatus || "Unknown");
    const cpuPriority = String(process.cpuPriority || "Unknown");
    const gpuPreference = String(process.gpuPreference || "Unknown");

    stats.total += 1;
    if (accessStatus === "Accessible") {
      stats.accessible += 1;
    } else {
      stats.protectedDenied += 1;
    }

    if (process.isFrozenByApp === true) {
      stats.frozenByApp += 1;
    }

    if (cpuPriority !== "Normal" && cpuPriority !== "Unknown") {
      stats.nonNormalPriority += 1;
    }

    if (gpuPreference !== "SystemDefault" && gpuPreference !== "Unknown") {
      stats.gpuPreferences += 1;
    }

    return stats;
  }, {
    total: 0,
    accessible: 0,
    protectedDenied: 0,
    frozenByApp: 0,
    nonNormalPriority: 0,
    gpuPreferences: 0,
  });
}

function statCard(label, value, hint, tone = "neutral") {
  const card = document.createElement("article");
  card.className = `panel stat-card ${tone}`;

  const valueElement = document.createElement("strong");
  valueElement.textContent = String(value);

  const labelElement = document.createElement("span");
  labelElement.textContent = label;

  const hintElement = document.createElement("p");
  hintElement.textContent = hint;

  card.appendChild(valueElement);
  card.appendChild(labelElement);
  card.appendChild(hintElement);
  return card;
}

function renderLastAction() {
  elements.lastActionContent.replaceChildren();

  if (!state.actionResult) {
    const empty = document.createElement("div");
    empty.className = "last-action-empty";
    empty.textContent = "No process actions performed in this session.";
    elements.lastActionContent.appendChild(empty);
    return;
  }

  elements.lastActionContent.appendChild(lastActionLine("Action", actionLabel(state.actionResult.action)));
  elements.lastActionContent.appendChild(lastActionLine("Status", state.actionResult.success ? "Succeeded" : "Failed", state.actionResult.success ? "success" : "danger"));
  elements.lastActionContent.appendChild(lastActionLine("Message", state.actionResult.message || "No backend message."));
}

function lastActionLine(label, value, tone) {
  const row = document.createElement("div");
  row.className = "last-action-line";

  const labelElement = document.createElement("span");
  labelElement.textContent = label;

  const valueElement = tone ? badge(value, tone) : document.createElement("strong");
  valueElement.textContent = value;
  if (!tone) {
    valueElement.title = value;
  }

  row.appendChild(labelElement);
  row.appendChild(valueElement);
  return row;
}

function actionLabel(action) {
  const labels = {
    setCpuPriority: "CPU Priority",
    terminateProcess: "End Process",
    freezeProcess: "Freeze",
    resumeProcess: "Resume",
    setGpuPreference: "GPU Preference",
  };
  return labels[action] || action || "Unknown action";
}

function renderRows() {
  elements.processRows.replaceChildren();

  if (state.filtered.length === 0) {
    const row = document.createElement("tr");
    const cell = document.createElement("td");
    cell.colSpan = 8;
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
    row.appendChild(badgeCell(gpuPreferenceLabel(process.gpuPreference), gpuPreferenceTone(process.gpuPreference)));
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
  elements.detailsContent.appendChild(gpuPreferenceSection(selected));
  elements.detailsContent.appendChild(section("Access status", [
    badge(selected.accessStatus || "Unknown", accessTone(selected.accessStatus)),
  ]));
  elements.detailsContent.appendChild(section("Admin requirement", [
    valueLine(selected.adminNeeded ? "Likely required for some future process actions" : "Not detected for this read-only snapshot"),
  ]));
  elements.detailsContent.appendChild(section("Safety model", [
    safetyNote("Critical Windows processes are blocked. Destructive actions require confirmation and target only the selected process."),
  ]));
  elements.detailsContent.appendChild(freezeResumeSection(selected));
  elements.detailsContent.appendChild(terminateSection(selected));

  if (selected.accessError) {
    elements.detailsContent.appendChild(section("Access error", [
      valueLine(selected.accessError, selected.accessError),
    ]));
  }
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

function gpuPreferenceSection(process) {
  const container = document.createElement("section");
  container.className = "detail-section gpu-preference-section";

  const heading = document.createElement("div");
  heading.className = "section-label";
  heading.textContent = "GPU preference";
  container.appendChild(heading);
  container.appendChild(badge(gpuPreferenceLabel(process.gpuPreference), gpuPreferenceTone(process.gpuPreference)));

  const info = document.createElement("div");
  info.className = "gpu-info";
  info.textContent = "GPU preference is applied per executable path and may require restarting the target app.";
  container.appendChild(info);

  const unavailableReason = getGpuPreferenceUnavailableReason(process);
  const form = document.createElement("div");
  form.className = "priority-form";

  if (unavailableReason) {
    const reason = document.createElement("div");
    reason.className = "priority-disabled-reason";
    reason.textContent = unavailableReason;
    form.appendChild(reason);
  }

  const select = document.createElement("select");
  select.className = "priority-select";
  const options = [
    ["SystemDefault", "System default"],
    ["PowerSaving", "Power saving / iGPU"],
    ["HighPerformance", "High performance / dGPU"],
  ];
  const currentPreference = normalizeGpuPreference(process.gpuPreference);
  for (const [value, label] of options) {
    const option = document.createElement("option");
    option.value = value;
    option.textContent = label;
    option.selected = value === currentPreference;
    select.appendChild(option);
  }

  const actions = document.createElement("div");
  actions.className = "gpu-actions";

  const applyButton = document.createElement("button");
  applyButton.className = "apply-priority-button";
  applyButton.type = "button";
  applyButton.textContent = state.pendingGpuPid === process.pid ? "Applying..." : "Apply GPU Preference";

  const resetButton = document.createElement("button");
  resetButton.className = "secondary-button";
  resetButton.type = "button";
  resetButton.textContent = "Reset to System default";

  const updateDisabledState = () => {
    const disabled = Boolean(unavailableReason) || state.pendingGpuPid === process.pid;
    select.disabled = disabled;
    applyButton.disabled = disabled;
    resetButton.disabled = disabled || currentPreference === "SystemDefault";
  };

  applyButton.addEventListener("click", () => {
    state.pendingGpuPid = process.pid;
    state.actionResult = null;
    render();
    postToHost({
      type: "setGpuPreference",
      pid: process.pid,
      expectedName: process.name || "",
      exePath: process.path || "",
      preference: select.value,
    });
  });

  resetButton.addEventListener("click", () => {
    state.pendingGpuPid = process.pid;
    state.actionResult = null;
    render();
    postToHost({
      type: "setGpuPreference",
      pid: process.pid,
      expectedName: process.name || "",
      exePath: process.path || "",
      preference: "SystemDefault",
    });
  });

  form.appendChild(select);
  actions.appendChild(applyButton);
  actions.appendChild(resetButton);
  form.appendChild(actions);
  container.appendChild(form);
  renderActionResult(container, process.pid, "setGpuPreference");
  updateDisabledState();
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

function getGpuPreferenceUnavailableReason(process) {
  const path = String(process.path || "").trim();
  if (!path || path === "Unavailable" || !path.toLowerCase().endsWith(".exe")) {
    return "GPU preference requires a valid executable path.";
  }

  if (process.pid === 0 || process.pid === 4 || process.name === "System" || process.accessStatus === "Protected/System") {
    return "Protected/system process GPU preference cannot be changed here.";
  }

  if (process.accessStatus === "Access denied" && !path) {
    return "GPU preference requires a valid executable path.";
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

function normalizeGpuPreference(preference) {
  if (preference === "PowerSaving" || preference === "HighPerformance" || preference === "SystemDefault") {
    return preference;
  }

  return "SystemDefault";
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

function safetyNote(value) {
  const div = document.createElement("div");
  div.className = "safety-note";
  div.textContent = value;
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

function gpuPreferenceLabel(preference) {
  if (preference === "PowerSaving") {
    return "Power saving";
  }
  if (preference === "HighPerformance") {
    return "High performance";
  }
  if (preference === "SystemDefault") {
    return "System default";
  }
  return "Unknown";
}

function gpuPreferenceTone(preference) {
  if (preference === "HighPerformance") {
    return "warning";
  }
  if (preference === "PowerSaving") {
    return "success";
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
elements.dashboardRefreshButton.addEventListener("click", requestProcesses);
elements.quickRefreshButton.addEventListener("click", requestProcesses);
elements.dashboardNavButton.addEventListener("click", () => setActiveView("dashboard"));
elements.processesNavButton.addEventListener("click", () => setActiveView("processes"));
elements.goToProcessesButton.addEventListener("click", () => setActiveView("processes"));
elements.quickProcessesButton.addEventListener("click", () => setActiveView("processes"));
elements.searchInput.addEventListener("input", (event) => {
  state.query = event.target.value;
  applyFilter();
});

render();
requestProcesses();
