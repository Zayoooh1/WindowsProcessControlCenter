const state = {
  processes: [],
  filtered: [],
  selectedPid: null,
  query: "",
  pendingPriorityPid: null,
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

  render();
}

function render() {
  elements.processCount.textContent = `${state.processes.length} processes`;
  elements.snapshotSummary.textContent = `${state.filtered.length} shown from ${state.processes.length} active processes`;
  renderRows();
  renderDetails();
}

function renderRows() {
  elements.processRows.replaceChildren();

  if (state.filtered.length === 0) {
    const row = document.createElement("tr");
    const cell = document.createElement("td");
    cell.colSpan = 6;
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
  elements.detailsContent.appendChild(cpuPrioritySection(selected));
  elements.detailsContent.appendChild(section("Access status", [
    badge(selected.accessStatus || "Unknown", accessTone(selected.accessStatus)),
  ]));
  elements.detailsContent.appendChild(section("Admin requirement", [
    valueLine(selected.adminNeeded ? "Likely required for some future process actions" : "Not detected for this read-only snapshot"),
  ]));

  if (selected.accessError) {
    elements.detailsContent.appendChild(section("Access error", [
      valueLine(selected.accessError, selected.accessError),
    ]));
  }

  const actions = document.createElement("div");
  actions.className = "future-actions";
  for (const label of ["End Process", "Freeze", "Resume", "Set GPU Preference"]) {
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

function renderActionResult(container, pid) {
  const existing = container.querySelector(".action-message");
  existing?.remove();

  if (!state.actionResult || state.actionResult.pid !== pid) {
    return;
  }

  const message = document.createElement("div");
  message.className = `action-message ${state.actionResult.success ? "success" : "error"}`;
  message.textContent = state.actionResult.message || (state.actionResult.success ? "Priority changed." : "Priority change failed.");
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

function showError(message) {
  elements.errorBanner.textContent = message;
  elements.errorBanner.classList.remove("hidden");
}

function hideError() {
  elements.errorBanner.classList.add("hidden");
  elements.errorBanner.textContent = "";
}

window.chrome?.webview?.addEventListener("message", handleHostMessage);
elements.refreshButton.addEventListener("click", requestProcesses);
elements.searchInput.addEventListener("input", (event) => {
  state.query = event.target.value;
  applyFilter();
});

requestProcesses();
