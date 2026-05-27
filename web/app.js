const SETTINGS_STORAGE_KEY = "wpcc.settings";
const VALID_UPDATE_INTERVALS = ["3d", "weekly", "monthly"];
const UPDATE_STATE_KEY = "wpcc.updateState";
const CURRENT_VERSION = "0.1.4";

const DEFAULT_SETTINGS = {
  startScreen: "dashboard",
  compactProcessTable: false,
  showExecutablePathColumn: true,
  showSafetyNotes: true,
  reduceVisualEffects: false,
  confirmDestructiveActions: true,
  updateChecksEnabled: true,
  updateCheckInterval: "weekly",
  autoInstallUpdates: false,
  ignoredUpdateVersion: null,
  autoRefreshInterval: "off",
  showPidColumn: true,
};

const VALID_AUTO_REFRESH_INTERVALS = ["off", "5s", "15s", "30s", "60s"];
const initialSettingsState = loadSettings();
const initialProfilesState = loadProfiles();
let autoRefreshTimer = null;
let snapshotDebounceTimer = null;

const state = {
  activeView: initialSettingsState.settings.startScreen,
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
  settings: initialSettingsState.settings,
  settingsStorageAvailable: initialSettingsState.storageAvailable,
  settingsStorageWarning: initialSettingsState.warning,
  resetSettingsModalOpen: false,
  detailsPanelOpen: true,
  pendingInstallerFilePath: null,
  // Sorting state
  sortColumn: null,
  sortDirection: "none",
  // Profiles State
  profilesState: {
    profiles: initialProfilesState.profiles,
    storageAvailable: initialProfilesState.storageAvailable,
    warning: initialProfilesState.warning,
    applyingProfileId: null,
  },
  editingProfileId: null,
  deleteModalProfile: null,
};

const elements = {
  dashboardNavButton: document.getElementById("dashboardNavButton"),
  processesNavButton: document.getElementById("processesNavButton"),
  settingsNavButton: document.getElementById("settingsNavButton"),
  aboutNavButton: document.getElementById("aboutNavButton"),
  rulesNavButton: document.getElementById("rulesNavButton"),
  dashboardView: document.getElementById("dashboardView"),
  processesView: document.getElementById("processesView"),
  settingsView: document.getElementById("settingsView"),
  aboutView: document.getElementById("aboutView"),
  rulesView: document.getElementById("rulesView"),
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
  settingsStorageNotice: document.getElementById("settingsStorageNotice"),
  startScreenDashboard: document.getElementById("startScreenDashboard"),
  startScreenProcesses: document.getElementById("startScreenProcesses"),
  compactTableToggle: document.getElementById("compactTableToggle"),
  showPathToggle: document.getElementById("showPathToggle"),
  showPidToggle: document.getElementById("showPidToggle"),
  showSafetyNotesToggle: document.getElementById("showSafetyNotesToggle"),
  confirmDestructiveToggle: document.getElementById("confirmDestructiveToggle"),
  reduceEffectsToggle: document.getElementById("reduceEffectsToggle"),
  resetSettingsButton: document.getElementById("resetSettingsButton"),
  updatesChecksToggle: document.getElementById("updatesChecksToggle"),
  updateIntervalSelect: document.getElementById("updateIntervalSelect"),
  autoRefreshSelect: document.getElementById("autoRefreshSelect"),
  autoInstallToggle: document.getElementById("autoInstallToggle"),
  ignoredUpdateVersionDisplay: document.getElementById("ignoredUpdateVersionDisplay"),
  resetIgnoredVersionButton: document.getElementById("resetIgnoredVersionButton"),
  manualCheckButton: document.getElementById("manualCheckButton"),
  updateStatusArea: document.getElementById("updateStatusArea"),
  downloadCompleteModal: document.getElementById("downloadCompleteModal"),
  dismissInstallBtn: document.getElementById("dismissInstallBtn"),
  confirmInstallBtn: document.getElementById("confirmInstallBtn"),
  searchInput: document.getElementById("searchInput"),
  processRows: document.getElementById("processRows"),
  detailsContent: document.getElementById("detailsContent"),
  detailsPanel: document.getElementById("detailsPanel"),
  toggleDetailsBtn: document.getElementById("toggleDetailsBtn"),
  closeDetailsButton: document.getElementById("closeDetailsButton"),
  errorBanner: document.getElementById("errorBanner"),
  // Profiles Elements
  rulesStorageNotice: document.getElementById("rulesStorageNotice"),
  rulesActionsBar: document.getElementById("rulesActionsBar"),
  createProfileButton: document.getElementById("createProfileButton"),
  exportProfilesButton: document.getElementById("exportProfilesButton"),
  importProfilesButton: document.getElementById("importProfilesButton"),
  importFileInput: document.getElementById("importFileInput"),
  rulesEmptyState: document.getElementById("rulesEmptyState"),
  emptyCreateProfileButton: document.getElementById("emptyCreateProfileButton"),
  profilesList: document.getElementById("profilesList"),
  profileModal: document.getElementById("profileModal"),
  profileModalTitle: document.getElementById("profileModalTitle"),
  profileForm: document.getElementById("profileForm"),
  profileName: document.getElementById("profileName"),
  profileRunningProcessPicker: document.getElementById("profileRunningProcessPicker"),
  profileMatchMode: document.getElementById("profileMatchMode"),
  profileExePath: document.getElementById("profileExePath"),
  profileProcessName: document.getElementById("profileProcessName"),
  exePathGroup: document.getElementById("exePathGroup"),
  processNameGroup: document.getElementById("processNameGroup"),
  profileCpuPriority: document.getElementById("profileCpuPriority"),
  profileGpuPreference: document.getElementById("profileGpuPreference"),
  profileRealtimeWarningBlock: document.getElementById("profileRealtimeWarningBlock"),
  profileRealtimeCheckbox: document.getElementById("profileRealtimeCheckbox"),
  profileApplyToFamily: document.getElementById("profileApplyToFamily"),
  profileAutoApply: document.getElementById("profileAutoApply"),
  profileNotes: document.getElementById("profileNotes"),
  profileResetButton: document.getElementById("profileResetButton"),
  profileCancelButton: document.getElementById("profileCancelButton"),
  profileSaveButton: document.getElementById("profileSaveButton"),
  deleteProfileModal: document.getElementById("deleteProfileModal"),
  colPriorityHeader: document.querySelector("th.col-priority"),
  colGpuHeader: document.querySelector("th.col-gpu"),
  deleteProfileNameDisplay: document.getElementById("deleteProfileNameDisplay"),
  deleteProfileTargetDisplay: document.getElementById("deleteProfileTargetDisplay"),
  deleteProfileCancelButton: document.getElementById("deleteProfileCancelButton"),
  deleteProfileConfirmButton: document.getElementById("deleteProfileConfirmButton"),
};

function loadSettings() {
  const fallback = {
    settings: { ...DEFAULT_SETTINGS },
    storageAvailable: false,
    warning: "",
  };

  try {
    const storage = window.localStorage;
    const testKey = `${SETTINGS_STORAGE_KEY}.test`;
    storage.setItem(testKey, "1");
    storage.removeItem(testKey);

    const raw = storage.getItem(SETTINGS_STORAGE_KEY);
    if (!raw) {
      return { ...fallback, storageAvailable: true };
    }

    try {
      const parsed = JSON.parse(raw);
      return {
        settings: normalizeSettings(parsed),
        storageAvailable: true,
        warning: "",
      };
    } catch {
      return {
        settings: { ...DEFAULT_SETTINGS },
        storageAvailable: true,
        warning: "Saved settings could not be read. Defaults are active until settings are saved again.",
      };
    }
  } catch {
    return {
      settings: { ...DEFAULT_SETTINGS },
      storageAvailable: false,
      warning: "Local settings storage is unavailable. Defaults are active for this session.",
    };
  }
}

function normalizeSettings(value) {
  const source = value && typeof value === "object" ? value : {};
  return {
    startScreen: source.startScreen === "processes" ? "processes" : "dashboard",
    compactProcessTable: Boolean(source.compactProcessTable),
    showExecutablePathColumn: source.showExecutablePathColumn !== false,
    showPidColumn: source.showPidColumn !== false,
    showSafetyNotes: source.showSafetyNotes !== false,
    reduceVisualEffects: Boolean(source.reduceVisualEffects),
    confirmDestructiveActions: true,
    updateChecksEnabled: source.updateChecksEnabled !== false,
    updateCheckInterval: VALID_UPDATE_INTERVALS.includes(source.updateCheckInterval)
      ? source.updateCheckInterval
      : "weekly",
    autoInstallUpdates: Boolean(source.autoInstallUpdates),
    ignoredUpdateVersion: source.ignoredUpdateVersion && typeof source.ignoredUpdateVersion === "string"
      ? source.ignoredUpdateVersion
      : null,
    autoRefreshInterval: VALID_AUTO_REFRESH_INTERVALS.includes(source.autoRefreshInterval)
      ? source.autoRefreshInterval
      : "off",
  };
}

function saveSettings() {
  if (!state.settingsStorageAvailable) {
    return;
  }

  try {
    window.localStorage.setItem(SETTINGS_STORAGE_KEY, JSON.stringify(state.settings));
    state.settingsStorageWarning = "";
  } catch {
    state.settingsStorageAvailable = false;
    state.settingsStorageWarning = "Local settings storage is unavailable. Defaults are active for this session.";
  }
}

// ==========================================
// PROFILES V1 MANAGEMENT LOGIC
// ==========================================

const PROFILES_STORAGE_KEY = "wpcc.profiles";

const VALID_PRIORITIES = ["DoNotChange", "High", "AboveNormal", "Normal", "BelowNormal", "Idle", "Realtime"];
const VALID_GPU_PREFERENCES = ["DoNotChange", "SystemDefault", "PowerSaving", "HighPerformance"];
const VALID_MATCH_MODES = ["path", "name"];

function loadProfiles() {
  return {
    schemaVersion: 1,
    profiles: [],
    storageAvailable: true,
    warning: "Loading profiles from native storage...",
  };
}

function normalizeProfiles(parsed) {
  const schemaVersion = parsed && typeof parsed.schemaVersion === "number" ? parsed.schemaVersion : 1;
  const rawProfiles = parsed && Array.isArray(parsed.profiles) ? parsed.profiles : [];

  const profiles = rawProfiles.map((p) => {
    if (!p || typeof p !== "object") return null;

    const id = typeof p.id === "string" && p.id ? p.id : `prof_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
    const name = typeof p.name === "string" ? p.name.trim() : "Unnamed Profile";
    const targetExePath = typeof p.targetExePath === "string" ? p.targetExePath.trim() : "";
    const targetProcessName = typeof p.targetProcessName === "string" ? p.targetProcessName.trim() : "";
    const matchMode = VALID_MATCH_MODES.includes(p.matchMode) ? p.matchMode : "path";
    const cpuPriority = VALID_PRIORITIES.includes(p.cpuPriority) ? p.cpuPriority : "DoNotChange";
    const gpuPreference = VALID_GPU_PREFERENCES.includes(p.gpuPreference) ? p.gpuPreference : "DoNotChange";
    const applyToFamily = Boolean(p.applyToFamily);
    const autoApply = false;
    const allowRealtime = Boolean(p.allowRealtime);
    const notes = typeof p.notes === "string" ? p.notes.trim() : "";
    const createdAt = typeof p.createdAt === "string" ? p.createdAt : new Date().toISOString();
    const updatedAt = typeof p.updatedAt === "string" ? p.updatedAt : new Date().toISOString();

    return {
      id,
      name,
      targetExePath,
      targetProcessName,
      matchMode,
      cpuPriority,
      gpuPreference,
      applyToFamily,
      autoApply,
      allowRealtime,
      notes,
      createdAt,
      updatedAt,
    };
  }).filter(Boolean);

  return {
    schemaVersion,
    profiles,
  };
}

function saveProfiles() {
  const data = {
    schemaVersion: 1,
    profiles: state.profilesState.profiles,
  };

  if (window.chrome?.webview) {
    window.chrome.webview.postMessage({
      type: "saveProfiles",
      profiles: JSON.stringify(data),
    });
  } else {
    state.profilesState.warning = "Cannot save: Native backend is unavailable.";
  }
}

function exportProfiles() {
  const data = {
    schemaVersion: 1,
    profiles: state.profilesState.profiles,
  };

  if (window.chrome?.webview) {
    window.chrome.webview.postMessage({
      type: "exportProfilesToFile",
      profiles: JSON.stringify(data),
    });
    return;
  }

  const blob = new Blob([JSON.stringify(data, null, 2)], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = "wpcc-profiles.json";
  document.body.appendChild(anchor);
  anchor.click();
  document.body.removeChild(anchor);
  URL.revokeObjectURL(url);
}

function importProfiles() {
  elements.importFileInput.value = "";
  elements.importFileInput.click();
}

function handleImportFile(event) {
  const file = event.target.files?.[0];
  if (!file) return;

  const reader = new FileReader();
  reader.onload = (e) => {
    try {
      const parsed = JSON.parse(e.target.result);
      if (!parsed || typeof parsed !== "object") {
        state.profilesState.warning = "Import failed: file does not contain a valid JSON object.";
        render();
        return;
      }
      const validated = normalizeProfiles(parsed);
      if (parsed.schemaVersion !== 1) {
        state.profilesState.warning = `Import failed: unsupported schema version "${parsed.schemaVersion}". Only version 1 is supported.`;
        render();
        return;
      }
      state.profilesState.profiles = validated.profiles;
      saveProfiles();
    } catch {
      state.profilesState.warning = "Import failed: file does not contain valid JSON.";
    }
    render();
  };
  reader.readAsText(file);
}

function openProfileModal(profileId = null) {
  state.editingProfileId = profileId;
  const prof = profileId ? state.profilesState.profiles.find(p => p.id === profileId) : null;

  elements.profileModalTitle.textContent = profileId ? "Edit profile" : "Create profile";
  
  elements.profileName.value = prof ? prof.name : "";
  elements.profileMatchMode.value = prof ? prof.matchMode : "path";
  elements.profileExePath.value = prof ? prof.targetExePath : "";
  elements.profileProcessName.value = prof ? prof.targetProcessName : "";
  elements.profileCpuPriority.value = prof ? prof.cpuPriority : "DoNotChange";
  elements.profileGpuPreference.value = prof ? prof.gpuPreference : "DoNotChange";
  elements.profileApplyToFamily.checked = prof ? prof.applyToFamily : false;
  elements.profileAutoApply.checked = false;
  elements.profileNotes.value = prof ? prof.notes : "";
  elements.profileRealtimeCheckbox.checked = prof ? prof.allowRealtime : false;

  if (elements.profileDeleteButton) {
    elements.profileDeleteButton.classList.toggle("hidden", !profileId);
  }

  populateRunningProcessPicker();

  updateMatchModeUi();
  updateCpuRealtimeUi();

  elements.profileModal.classList.remove("hidden-view");
  elements.profileName.focus();
}

function closeProfileModal() {
  state.editingProfileId = null;
  elements.profileModal.classList.add("hidden-view");
  if (elements.profilesList.classList.contains("hidden-view") || state.profilesState.profiles.length === 0) {
    elements.emptyCreateProfileButton.focus();
  } else {
    elements.createProfileButton.focus();
  }
}

function resetProfileForm() {
  if (state.editingProfileId) {
    openProfileModal(state.editingProfileId);
  } else {
    elements.profileName.value = "";
    elements.profileMatchMode.value = "path";
    elements.profileExePath.value = "";
    elements.profileProcessName.value = "";
    elements.profileCpuPriority.value = "DoNotChange";
    elements.profileGpuPreference.value = "DoNotChange";
    elements.profileApplyToFamily.checked = false;
    elements.profileAutoApply.checked = false;
    elements.profileNotes.value = "";
    elements.profileRealtimeCheckbox.checked = false;

    if (elements.profileRunningProcessPicker) {
      elements.profileRunningProcessPicker.selectedIndex = 0;
    }

    updateMatchModeUi();
    updateCpuRealtimeUi();
    elements.profileName.focus();
  }
}

function saveProfileForm(event) {
  event.preventDefault();

  const priority = elements.profileCpuPriority.value;
  const isRealtime = priority === "Realtime";
  if (isRealtime && !elements.profileRealtimeCheckbox.checked) {
    return;
  }

  const now = new Date().toISOString();
  let prof = null;

  if (state.editingProfileId) {
    prof = state.profilesState.profiles.find(p => p.id === state.editingProfileId);
  }

  const profileData = {
    id: prof ? prof.id : `prof_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
    name: elements.profileName.value.trim() || "Unnamed Profile",
    targetExePath: elements.profileExePath.value.trim(),
    targetProcessName: elements.profileProcessName.value.trim(),
    matchMode: elements.profileMatchMode.value,
    cpuPriority: priority,
    gpuPreference: elements.profileGpuPreference.value,
    applyToFamily: elements.profileApplyToFamily.checked,
    autoApply: false,
    allowRealtime: isRealtime && elements.profileRealtimeCheckbox.checked,
    notes: elements.profileNotes.value.trim(),
    createdAt: prof ? prof.createdAt : now,
    updatedAt: now
  };

  if (state.editingProfileId) {
    state.profilesState.profiles = state.profilesState.profiles.map(p => p.id === state.editingProfileId ? profileData : p);
  } else {
    state.profilesState.profiles.push(profileData);
  }

  saveProfiles();
  closeProfileModal();
  render();

  const matches = getMatchingProcesses(profileData);
  if (matches.length > 0 && (profileData.applyToFamily || profileData.targetExePath || profileData.targetProcessName)) {
    applyProfile(profileData.id);
  }
}

function openDeleteProfileModal(profile) {
  state.deleteModalProfile = profile;
  elements.deleteProfileNameDisplay.textContent = profile.name;
  elements.deleteProfileTargetDisplay.textContent = profile.matchMode === "path" 
    ? (profile.targetExePath || "Any Executable") 
    : (profile.targetProcessName || "Any Process");
  elements.deleteProfileModal.classList.remove("hidden-view");
  elements.deleteProfileCancelButton.focus();
}

function closeDeleteProfileModal() {
  state.deleteModalProfile = null;
  elements.deleteProfileModal.classList.add("hidden-view");
}

function confirmDeleteProfile() {
  if (!state.deleteModalProfile) return;
  state.profilesState.profiles = state.profilesState.profiles.filter(p => p.id !== state.deleteModalProfile.id);
  saveProfiles();
  closeDeleteProfileModal();
  render();
}

function updateMatchModeUi() {
  const isPath = elements.profileMatchMode.value === "path";
  elements.exePathGroup.classList.toggle("hidden", !isPath);
  elements.processNameGroup.classList.toggle("hidden", isPath);
}

function updateCpuRealtimeUi() {
  const isRealtime = elements.profileCpuPriority.value === "Realtime";
  elements.profileRealtimeWarningBlock.classList.toggle("hidden-view", !isRealtime);
  
  const canSave = !isRealtime || elements.profileRealtimeCheckbox.checked;
  elements.profileSaveButton.disabled = !canSave;
}

function getMatchingProcesses(profile) {
  return state.processes.filter(function (process) {
    if (profile.matchMode === "path" && profile.targetExePath) {
      return process.path && process.path.toLowerCase() === profile.targetExePath.toLowerCase();
    }
    if (profile.matchMode === "name" && profile.targetProcessName) {
      return process.name && process.name.toLowerCase() === profile.targetProcessName.toLowerCase();
    }
    return false;
  });
}

function applyProfile(profileId) {
  const profile = state.profilesState.profiles.find(p => p.id === profileId);
  if (!profile) return;

  if (profile.cpuPriority === "Realtime" && !profile.allowRealtime) {
    state.profilesState.warning = "Cannot apply: Realtime priority requires confirmation in the profile settings. Edit the profile and confirm the Realtime risk to enable it.";
    render();
    return;
  }

  state.profilesState.applyingProfileId = profileId;
  render();

  postToHost({
    type: "applyProfile",
    profileId: profileId,
  });
}

function renderProfiles() {
  const hasWarning = Boolean(state.profilesState.warning);
  elements.rulesStorageNotice.classList.toggle("hidden", !hasWarning);
  if (hasWarning) {
    elements.rulesStorageNotice.textContent = state.profilesState.warning;
  }

  const profiles = state.profilesState.profiles;
  const hasProfiles = profiles.length > 0;

  elements.rulesEmptyState.classList.toggle("hidden-view", hasProfiles);
  elements.rulesActionsBar.classList.remove("hidden-view");
  elements.profilesList.classList.toggle("hidden-view", !hasProfiles);

  if (!hasProfiles) {
    return;
  }

  elements.profilesList.replaceChildren();

  for (const prof of profiles) {
    const row = document.createElement("div");
    row.className = "profile-row";
    row.setAttribute("role", "button");
    row.setAttribute("tabindex", "0");
    row.addEventListener("click", () => {
      openProfileModal(prof.id);
    });
    row.addEventListener("keydown", (e) => {
      if (e.key === "Enter" || e.key === " ") {
        e.preventDefault();
        openProfileModal(prof.id);
      }
    });

    const leftContainer = document.createElement("div");
    leftContainer.className = "profile-row-left";

    const iconSpan = document.createElement("span");
    iconSpan.className = "profile-row-icon";
    iconSpan.textContent = "⚡";

    const titleSpan = document.createElement("span");
    titleSpan.className = "profile-row-title";
    titleSpan.textContent = prof.name;

    leftContainer.appendChild(iconSpan);
    leftContainer.appendChild(titleSpan);
    row.appendChild(leftContainer);

    const rightContainer = document.createElement("div");
    rightContainer.className = "profile-row-right";

    const cpuBadgeVal = prof.cpuPriority === "DoNotChange" ? "CPU: Keep Current" : `CPU: ${prof.cpuPriority}`;
    const cpuBadge = badge(cpuBadgeVal, prof.cpuPriority === "DoNotChange" ? "neutral" : priorityTone(prof.cpuPriority));
    rightContainer.appendChild(cpuBadge);

    const gpuBadgeVal = prof.gpuPreference === "DoNotChange" 
      ? "GPU: Keep Current" 
      : `GPU: ${gpuPreferenceLabel(prof.gpuPreference)}`;
    const gpuBadge = badge(gpuBadgeVal, prof.gpuPreference === "DoNotChange" ? "neutral" : gpuPreferenceTone(prof.gpuPreference));
    rightContainer.appendChild(gpuBadge);

    row.appendChild(rightContainer);
    elements.profilesList.appendChild(row);
  }
}

function postToHost(message) {
  if (!window.chrome?.webview) {
    showError("WebView2 bridge is not available.");
    return;
  }

  window.chrome.webview.postMessage(message);
}

function requestNativeProfiles() {
  if (!window.chrome?.webview) return;
  window.chrome.webview.postMessage({ type: "getProfiles" });
}

function requestProcesses() {
  hideError();
  elements.refreshButton.disabled = true;
  elements.dashboardRefreshButton.disabled = true;
  elements.quickRefreshButton.disabled = true;
  postToHost({ type: "refreshProcesses" });
}

function startAutoRefresh() {
  stopAutoRefresh();
  const value = state.settings.autoRefreshInterval;
  const intervalMap = { "5s": 5000, "15s": 15000, "30s": 30000, "60s": 60000 };
  const ms = intervalMap[value];
  if (!ms) return;
  autoRefreshTimer = setInterval(requestProcesses, ms);
}

function stopAutoRefresh() {
  if (autoRefreshTimer !== null) {
    clearInterval(autoRefreshTimer);
    autoRefreshTimer = null;
  }
}

function restartAutoRefresh() {
  if (state.settings.autoRefreshInterval !== "off") {
    startAutoRefresh();
  } else {
    stopAutoRefresh();
  }
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
    if (state.processes.length > 50) {
      clearTimeout(snapshotDebounceTimer);
      snapshotDebounceTimer = setTimeout(() => {
        applyFilter();
      }, 150);
    } else {
      applyFilter();
    }
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

  if (message.type === "profilesLoaded") {
    if (message.success && message.profiles && typeof message.profiles === "object") {
      const validated = normalizeProfiles(message.profiles);
      state.profilesState.profiles = validated.profiles;
      state.profilesState.warning = "";
    } else if (message.warning) {
      state.profilesState.warning = message.warning;
    } else if (!message.success) {
      state.profilesState.warning = "Failed to load profiles from native storage.";
    }
    render();
    return;
  }

  if (message.type === "profileApplied") {
    state.profilesState.applyingProfileId = null;
    showStatus(message.message || "Profile applied.", Boolean(message.success));
    requestProcesses();
    render();
    return;
  }

  if (message.type === "profilesSaved") {
    if (!message.success && message.warning) {
      state.profilesState.warning = message.warning;
      render();
    }
    return;
  }

  if (message.type === "profilesExported") {
    if (message.success) {
      state.profilesState.warning = "";
    } else if (!message.cancelled && message.warning) {
      state.profilesState.warning = message.warning;
      render();
    }
    return;
  }

  if (message.type === "error") {
    elements.refreshButton.disabled = false;
    elements.dashboardRefreshButton.disabled = false;
    elements.quickRefreshButton.disabled = false;
    showError(message.message || "Unknown backend error.");
    return;
  }

  if (message.type === "downloadProgress") {
    const progressBar = document.getElementById("updateProgressBar");
    const progressText = document.getElementById("updateProgressText");
    if (progressBar && progressText) {
      const percent = message.totalBytes > 0 ? Math.floor((message.downloadedBytes / message.totalBytes) * 100) : 0;
      progressBar.style.width = percent + "%";
      const downloadedMb = (message.downloadedBytes / (1024 * 1024)).toFixed(2);
      const totalMb = (message.totalBytes / (1024 * 1024)).toFixed(2);
      progressText.textContent = `Downloading... ${downloadedMb} MB / ${totalMb} MB (${percent}%)`;
    }
    return;
  }

  if (message.type === "downloadComplete") {
    const progressContainer = document.getElementById("updateProgressContainer");
    const completionActions = document.getElementById("updateCompletionActions");
    const updateProgressText = document.getElementById("updateProgressText");
    const actions = document.querySelector(".update-modal-actions");

    if (message.success === false) {
      if (progressContainer) {
        progressContainer.classList.add("hidden-view");
      }
      if (actions) {
        actions.classList.remove("hidden-view");
      }
      showError(message.message || "Failed to download update.");
    } else {
      state.pendingInstallerFilePath = message.filePath;
      if (progressContainer && completionActions && updateProgressText) {
        updateProgressText.textContent = "Download completed. Install now?";
        document.getElementById("updateProgressBar").style.width = "100%";
        completionActions.classList.remove("hidden-view");
      } else {
        elements.downloadCompleteModal.classList.remove("hidden-view");
      }
    }
    return;
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

  if (state.sortColumn && state.sortDirection !== "none") {
    const dirMultiplier = state.sortDirection === "asc" ? 1 : -1;
    state.filtered.sort((a, b) => {
      if (state.sortColumn === "pid") {
        return ((a.pid || 0) - (b.pid || 0)) * dirMultiplier;
      }
      if (state.sortColumn === "name") {
        return (a.name || "").localeCompare(b.name || "") * dirMultiplier;
      }
      if (state.sortColumn === "path") {
        return (a.path || "").localeCompare(b.path || "") * dirMultiplier;
      }
      if (state.sortColumn === "runtime") {
        return (runtimeLabel(a) || "").localeCompare(runtimeLabel(b) || "") * dirMultiplier;
      }
      if (state.sortColumn === "priority") {
        return (mapPriorityToValue(a.cpuPriority) - mapPriorityToValue(b.cpuPriority)) * dirMultiplier;
      }
      if (state.sortColumn === "gpu") {
        return (mapGpuToValue(a.gpuPreference) - mapGpuToValue(b.gpuPreference)) * dirMultiplier;
      }
      if (state.sortColumn === "admin") {
        const aVal = a.adminNeeded ? 1 : 0;
        const bVal = b.adminNeeded ? 1 : 0;
        return (aVal - bVal) * dirMultiplier;
      }
      if (state.sortColumn === "access") {
        return (a.accessStatus || "").localeCompare(b.accessStatus || "") * dirMultiplier;
      }
      return 0;
    });
  }

  if (!state.filtered.some((process) => process.pid === state.selectedPid)) {
    state.selectedPid = state.filtered[0]?.pid ?? null;
  }

  render();
}

function updateHeaderIndicators() {
  document.querySelectorAll("th.clickable-header").forEach(th => {
    th.classList.remove("sort-asc", "sort-desc");
    if (th.dataset.sort === state.sortColumn) {
      if (state.sortDirection === "asc") th.classList.add("sort-asc");
      else if (state.sortDirection === "desc") th.classList.add("sort-desc");
    }
  });
}

function render() {
  elements.processCount.textContent = `${state.processes.length} processes`;
  elements.snapshotSummary.textContent = `${state.filtered.length} shown from ${state.processes.length} active processes`;
  elements.processesView.classList.toggle("details-collapsed", !state.detailsPanelOpen);
  applySettingsEffects();
  updateHeaderIndicators();
  renderActiveView();
  renderDashboard();
  renderSettings();
  renderRows();
  renderDetails();
  renderTerminateModal();
  renderFreezeModal();
  renderResetSettingsModal();
  renderProfiles();
}

function setActiveView(view) {
  state.activeView = view;
  render();
}

function renderActiveView() {
  const dashboardActive = state.activeView === "dashboard";
  const processesActive = state.activeView === "processes";
  const settingsActive = state.activeView === "settings";
  const aboutActive = state.activeView === "about";
  const rulesActive = state.activeView === "rules";
  elements.dashboardView.classList.toggle("hidden-view", !dashboardActive);
  elements.processesView.classList.toggle("hidden-view", !processesActive);
  elements.settingsView.classList.toggle("hidden-view", !settingsActive);
  elements.aboutView.classList.toggle("hidden-view", !aboutActive);
  elements.rulesView.classList.toggle("hidden-view", !rulesActive);
  elements.dashboardNavButton.classList.toggle("active", dashboardActive);
  elements.processesNavButton.classList.toggle("active", processesActive);
  elements.settingsNavButton.classList.toggle("active", settingsActive);
  elements.aboutNavButton.classList.toggle("active", aboutActive);
  elements.rulesNavButton.classList.toggle("active", rulesActive);
}

function applySettingsEffects() {
  document.body.classList.toggle("compact-process-table", state.settings.compactProcessTable);
  document.body.classList.toggle("hide-executable-path", !state.settings.showExecutablePathColumn);
  document.body.classList.toggle("hide-pid-column", !state.settings.showPidColumn);
  document.body.classList.toggle("reduced-motion", state.settings.reduceVisualEffects);
}

function updateSetting(key, value) {
  state.settings = normalizeSettings({
    ...state.settings,
    [key]: value,
  });
  saveSettings();
  render();
}

function renderSettings() {
  elements.settingsStorageNotice.classList.toggle("hidden", !state.settingsStorageWarning);
  elements.settingsStorageNotice.textContent = state.settingsStorageWarning;
  elements.startScreenDashboard.checked = state.settings.startScreen === "dashboard";
  elements.startScreenProcesses.checked = state.settings.startScreen === "processes";
  elements.compactTableToggle.checked = state.settings.compactProcessTable;
  elements.showPathToggle.checked = state.settings.showExecutablePathColumn;
  elements.showPidToggle.checked = state.settings.showPidColumn;
  elements.showSafetyNotesToggle.checked = state.settings.showSafetyNotes;
  elements.reduceEffectsToggle.checked = state.settings.reduceVisualEffects;
  elements.confirmDestructiveToggle.checked = true;
  elements.confirmDestructiveToggle.disabled = true;
  elements.updatesChecksToggle.checked = state.settings.updateChecksEnabled;
  elements.updateIntervalSelect.value = state.settings.updateCheckInterval;
  elements.updateIntervalSelect.disabled = !state.settings.updateChecksEnabled;
  elements.autoRefreshSelect.value = state.settings.autoRefreshInterval;
  elements.autoInstallToggle.checked = state.settings.autoInstallUpdates;
  elements.autoInstallToggle.disabled = false;
  elements.ignoredUpdateVersionDisplay.textContent = state.settings.ignoredUpdateVersion || "None";
  elements.resetIgnoredVersionButton.disabled = !state.settings.ignoredUpdateVersion;
  elements.manualCheckButton.disabled = false;
  renderUpdateStatus();
}

function loadUpdateState() {
  try {
    const raw = window.localStorage.getItem(UPDATE_STATE_KEY);
    if (!raw) return {
      lastCheckedAt: null,
      lastKnownVersion: null,
      latestReleaseUrl: null,
      ignoredVersion: state.settings.ignoredUpdateVersion || null,
    };
    const parsed = JSON.parse(raw);
    return {
      lastCheckedAt: parsed.lastCheckedAt || null,
      lastKnownVersion: parsed.lastKnownVersion || null,
      latestReleaseUrl: parsed.latestReleaseUrl || null,
      ignoredVersion: parsed.ignoredVersion ?? (state.settings.ignoredUpdateVersion || null),
    };
  } catch (e) {
    return {
      lastCheckedAt: null,
      lastKnownVersion: null,
      latestReleaseUrl: null,
      ignoredVersion: state.settings.ignoredUpdateVersion || null,
    };
  }
}

function saveUpdateState(obj) {
  try {
    window.localStorage.setItem(UPDATE_STATE_KEY, JSON.stringify(obj));
  } catch (e) {
    // ignore storage failures
  }
}

function parseSemVer(input) {
  if (!input || typeof input !== "string") return null;
  const s = input.trim().replace(/^v/i, "");
  const m = s.match(/^(\d+)\.(\d+)\.(\d+)(?:[-+].*)?$/);
  if (!m) return null;
  return { major: Number(m[1]), minor: Number(m[2]), patch: Number(m[3]) };
}

function compareSemVer(aStr, bStr) {
  const a = parseSemVer(aStr);
  const b = parseSemVer(bStr);
  if (!a || !b) return null;
  if (a.major !== b.major) return a.major < b.major ? -1 : 1;
  if (a.minor !== b.minor) return a.minor < b.minor ? -1 : 1;
  if (a.patch !== b.patch) return a.patch < b.patch ? -1 : 1;
  return 0;
}

function formatDateIso(d) {
  try {
    return new Date(d).toLocaleString();
  } catch {
    return String(d);
  }
}

function renderUpdateStatus() {
  if (!elements.updateStatusArea) return;
  const stateObj = loadUpdateState();
  if (!stateObj.lastCheckedAt) {
    elements.updateStatusArea.textContent = "Never checked";
    return;
  }

  const latest = stateObj.lastKnownVersion || "Unknown";
  let status = "Last checked: " + formatDateIso(stateObj.lastCheckedAt) + " — ";
  const cmp = compareSemVer(CURRENT_VERSION, latest);
  if (stateObj.lastKnownVersion === null) {
    status += "Failed to determine latest release.";
  } else if (stateObj.ignoredVersion && stateObj.ignoredVersion === stateObj.lastKnownVersion) {
    status += `Latest: ${latest} (ignored)`;
  } else if (cmp === null) {
    status += `Latest: ${latest} (unable to compare versions)`;
  } else if (cmp >= 0) {
    status += `You are up to date.`;
  } else {
    status += `New version available: ${latest}`;
  }

  elements.updateStatusArea.textContent = status;
}

async function checkForUpdates(manual = false) {
  if (!elements.updateStatusArea) return;
  elements.updateStatusArea.textContent = "Checking for updates...";

  const abort = new AbortController();
  const timeout = setTimeout(() => abort.abort(), 8000);
  let updateState = loadUpdateState();
  try {
    const resp = await fetch("https://api.github.com/repos/Zayoooh1/WindowsProcessControlCenter/releases/latest", {
      method: "GET",
      headers: { Accept: "application/vnd.github.v3+json" },
      signal: abort.signal,
    });
    clearTimeout(timeout);

    if (!resp.ok) {
      // Provide a friendly message for 404 which commonly means no public release
      if (resp.status === 404) {
        elements.updateStatusArea.textContent = "Failed to check for updates. The GitHub release endpoint returned 404. This may happen when the repository is private or no public release exists.";
      } else {
        const text = await resp.text().catch(() => "");
        elements.updateStatusArea.textContent = `Failed to check for updates: ${resp.status} ${resp.statusText}`;
      }
      updateState.lastCheckedAt = new Date().toISOString();
      saveUpdateState(updateState);
      return;
    }

    const json = await resp.json().catch(() => null);
    if (!json) {
      elements.updateStatusArea.textContent = "Failed to parse update information.";
      updateState.lastCheckedAt = new Date().toISOString();
      saveUpdateState(updateState);
      return;
    }

    if (json.prerelease) {
      elements.updateStatusArea.textContent = "No stable release found.";
      updateState.lastCheckedAt = new Date().toISOString();
      saveUpdateState(updateState);
      return;
    }

    const tag = json.tag_name || json.name || null;
    const parsedVersion = tag ? tag.replace(/^v/i, "") : null;
    const releaseUrl = json.html_url || null;
    const releaseName = json.name || json.tag_name || "";

    // detect assets
    let installerExe = null;
    let portableZip = null;
    if (Array.isArray(json.assets)) {
      for (const asset of json.assets) {
        const name = String(asset.name || "").toLowerCase();
        if (!installerExe && name.endsWith('.exe')) installerExe = asset.browser_download_url || null;
        if (!portableZip && (name.endsWith('.zip') || name.endsWith('.portable.zip'))) portableZip = asset.browser_download_url || null;
      }
    }

    updateState.lastCheckedAt = new Date().toISOString();
    updateState.lastKnownVersion = parsedVersion || null;
    updateState.latestReleaseUrl = releaseUrl;
    updateState.ignoredVersion = updateState.ignoredVersion || state.settings.ignoredUpdateVersion || null;
    saveUpdateState(updateState);

    // render status
    if (!parsedVersion) {
      elements.updateStatusArea.textContent = `Checked ${formatDateIso(updateState.lastCheckedAt)} — unable to parse latest version.`;
      return;
    }

    const cmp = compareSemVer(CURRENT_VERSION, parsedVersion);
    if (cmp === null) {
      elements.updateStatusArea.textContent = `Checked ${formatDateIso(updateState.lastCheckedAt)} — latest ${parsedVersion}. (Comparison unavailable)`;
      return;
    }

    if (updateState.ignoredVersion && updateState.ignoredVersion === parsedVersion) {
      renderUpdateStatus();
      return;
    }

    if (cmp <= -1) {
      // latest is greater
      renderUpdateStatus();

      // Auto updates download
      if (state.settings.autoInstallUpdates) {
        const downloadUrl = installerExe || portableZip;
        if (downloadUrl) {
          postToHost({
            type: "downloadUpdate",
            url: downloadUrl,
          });
        }
      }

      // show modal with release details
      try {
        showUpdateModal({
          parsedVersion,
          releaseName,
          releaseUrl,
          installerExe,
          portableZip,
          body: json.body || "",
        });
      } catch (e) {
        // ignore modal errors
      }
    } else {
      renderUpdateStatus();
    }
  } catch (e) {
    clearTimeout(timeout);
    if (e && e.name === 'AbortError') {
      elements.updateStatusArea.textContent = 'Update check timed out.';
    } else {
      elements.updateStatusArea.textContent = 'Failed to check for updates.';
    }
    updateState.lastCheckedAt = new Date().toISOString();
    saveUpdateState(updateState);
  }
}

function shouldCheckByInterval(updateState) {
  if (!updateState || !updateState.lastCheckedAt) return true;
  const last = new Date(updateState.lastCheckedAt);
  if (Number.isNaN(last.getTime())) return true;
  const interval = state.settings.updateCheckInterval || 'weekly';
  const days = interval === '3d' ? 3 : interval === 'monthly' ? 30 : 7;
  const next = new Date(last.getTime() + days * 24 * 60 * 60 * 1000);
  return new Date() >= next;
}

function runAutoUpdateCheckIfNeeded() {
  try {
    if (!state.settings.updateChecksEnabled) return;
    const updateState = loadUpdateState();
    if (!shouldCheckByInterval(updateState)) return;
    // don't block UI
    setTimeout(() => checkForUpdates(false), 100);
  } catch (e) {
    // swallow
  }
}

function closeUpdateModal() {
  document.querySelector(".update-modal-backdrop")?.remove();
}

function showUpdateModal(release) {
  // release: { parsedVersion, releaseName, releaseUrl, installerExe, portableZip, body }
  closeUpdateModal();

  const backdrop = document.createElement("div");
  backdrop.className = "modal-backdrop update-modal-backdrop";

  const modal = document.createElement("div");
  modal.className = "confirm-modal update-modal";
  modal.setAttribute("role", "dialog");
  modal.setAttribute("aria-modal", "true");
  modal.setAttribute("aria-label", "Update available");

  const title = document.createElement("h2");
  title.textContent = "Update available";
  modal.appendChild(title);

  const info = document.createElement("div");
  info.className = "update-info";
  info.innerHTML = `
    <p>Current version: <strong>${CURRENT_VERSION}</strong></p>
    <p>Latest version: <strong>${release.parsedVersion}</strong></p>
  `;
  if (release.releaseName) {
    const rn = document.createElement("p");
    rn.textContent = `Release: ${release.releaseName}`;
    info.appendChild(rn);
  }
  if (release.body) {
    const body = document.createElement("p");
    const summary = String(release.body).split('\n').slice(0,3).join(' ');
    body.textContent = summary;
    body.className = "update-notes";
    info.appendChild(body);
  }

  modal.appendChild(info);

  const actions = document.createElement("div");
  actions.className = "modal-actions update-modal-actions";

  const openRelease = document.createElement("button");
  openRelease.type = "button";
  openRelease.className = "primary-button";
  openRelease.textContent = "Open release page";
  openRelease.addEventListener("click", () => {
    if (release.releaseUrl) window.open(release.releaseUrl, "_blank");
  });

  const downloadInstaller = document.createElement("button");
  downloadInstaller.type = "button";
  downloadInstaller.className = "secondary-button";
  downloadInstaller.textContent = "Download installer";
  downloadInstaller.disabled = !release.installerExe;
  downloadInstaller.addEventListener("click", (event) => {
    event.preventDefault();
    if (release.installerExe) {
      postToHost({
        type: "downloadUpdate",
        url: release.installerExe,
      });
      actions.classList.add("hidden-view");
      document.getElementById("updateProgressContainer").classList.remove("hidden-view");
    }
  });

  const downloadZip = document.createElement("button");
  downloadZip.type = "button";
  downloadZip.className = "secondary-button";
  downloadZip.textContent = "Download portable ZIP";
  downloadZip.disabled = !release.portableZip;
  downloadZip.addEventListener("click", (event) => {
    event.preventDefault();
    if (release.portableZip) {
      postToHost({
        type: "downloadUpdate",
        url: release.portableZip,
      });
      actions.classList.add("hidden-view");
      document.getElementById("updateProgressContainer").classList.remove("hidden-view");
    }
  });

  const remindLater = document.createElement("button");
  remindLater.type = "button";
  remindLater.className = "secondary-button";
  remindLater.textContent = "Remind me later";
  remindLater.addEventListener("click", () => {
    closeUpdateModal();
  });

  const ignoreVersion = document.createElement("button");
  ignoreVersion.type = "button";
  ignoreVersion.className = "warning-action-button";
  ignoreVersion.textContent = "Ignore this version";
  ignoreVersion.addEventListener("click", () => {
    // persist ignored version to both settings and updateState
    state.settings.ignoredUpdateVersion = release.parsedVersion;
    saveSettings();
    const us = loadUpdateState();
    us.ignoredVersion = release.parsedVersion;
    saveUpdateState(us);
    render();
    closeUpdateModal();
  });

  const disableChecks = document.createElement("button");
  disableChecks.type = "button";
  disableChecks.className = "danger-action-button";
  disableChecks.textContent = "Disable update checks";
  disableChecks.addEventListener("click", () => {
    state.settings.updateChecksEnabled = false;
    saveSettings();
    render();
    closeUpdateModal();
  });

  actions.appendChild(openRelease);
  actions.appendChild(downloadInstaller);
  actions.appendChild(downloadZip);
  actions.appendChild(remindLater);
  actions.appendChild(ignoreVersion);
  actions.appendChild(disableChecks);

  modal.appendChild(actions);

  const progressContainer = document.createElement("div");
  progressContainer.className = "progress-container hidden-view";
  progressContainer.id = "updateProgressContainer";
  progressContainer.innerHTML = `
    <div class="progress-track">
      <div class="progress-fill" id="updateProgressBar"></div>
    </div>
    <p id="updateProgressText" style="text-align: center; margin-top: 10px; color: var(--muted);">Starting download...</p>
  `;

  const completionActions = document.createElement("div");
  completionActions.className = "modal-actions hidden-view";
  completionActions.id = "updateCompletionActions";

  const confirmInstall = document.createElement("button");
  confirmInstall.type = "button";
  confirmInstall.className = "primary-button";
  confirmInstall.textContent = "Ok";
  confirmInstall.addEventListener("click", () => {
    if (state.pendingInstallerFilePath) {
      postToHost({
        type: "executeInstaller",
        filePath: state.pendingInstallerFilePath,
      });
      closeUpdateModal();
    }
  });

  const cancelInstall = document.createElement("button");
  cancelInstall.type = "button";
  cancelInstall.className = "secondary-button";
  cancelInstall.textContent = "Cancel";
  cancelInstall.addEventListener("click", () => {
    closeUpdateModal();
  });

  completionActions.appendChild(confirmInstall);
  completionActions.appendChild(cancelInstall);

  modal.appendChild(progressContainer);
  modal.appendChild(completionActions);

  backdrop.appendChild(modal);
  document.body.appendChild(backdrop);
  openRelease.focus();
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
  if (state.filtered.length === 0) {
    elements.processRows.replaceChildren();
    const row = document.createElement("tr");
    const cell = document.createElement("td");
    cell.colSpan = 8;
    cell.className = "empty-cell";
    cell.textContent = state.processes.length === 0 ? "No process snapshot loaded." : "No processes match the current search.";
    row.appendChild(cell);
    elements.processRows.appendChild(row);
    return;
  }

  const existingRows = Array.from(elements.processRows.children);
  const rowsByPid = new Map();
  for (const row of existingRows) {
    if (row.dataset.pid) rowsByPid.set(row.dataset.pid, row);
    else row.remove();
  }

  const fragment = document.createDocumentFragment();

  for (const process of state.filtered) {
    const pidStr = String(process.pid);
    let row = rowsByPid.get(pidStr);

    if (row) {
      rowsByPid.delete(pidStr);
      row.className = process.pid === state.selectedPid ? "selected" : "";
      row.replaceChildren();
    } else {
      row = document.createElement("tr");
      row.dataset.pid = pidStr;
      row.className = process.pid === state.selectedPid ? "selected" : "";
      row.addEventListener("click", () => {
        state.selectedPid = process.pid;
        if (!state.detailsPanelOpen) {
          state.detailsPanelOpen = true;
        }
        render();
      });
    }

    row.appendChild(textCell(process.pid, "col-pid pid-cell"));
    row.appendChild(textCell(process.name || "Unknown", "col-process"));
    row.appendChild(pathCell(process.path || "Unavailable"));
    row.appendChild(badgeCell(runtimeLabel(process), runtimeTone(process), "col-runtime"));
    row.appendChild(badgeCell(process.cpuPriority || "Unknown", priorityTone(process.cpuPriority), "col-priority"));
    row.appendChild(badgeCell(gpuPreferenceLabel(process.gpuPreference), gpuPreferenceTone(process.gpuPreference), "col-gpu"));
    row.appendChild(badgeCell(process.adminNeeded ? "Likely" : "No", process.adminNeeded ? "warning" : "neutral", "col-admin"));
    row.appendChild(badgeCell(process.accessStatus || "Unknown", accessTone(process.accessStatus), "col-access"));
    fragment.appendChild(row);
  }

  for (const row of rowsByPid.values()) {
    row.remove();
  }

  elements.processRows.replaceChildren(fragment);
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
  if (state.settings.showSafetyNotes) {
    elements.detailsContent.appendChild(section("Safety model", [
      safetyNote("Critical Windows processes are blocked. Destructive actions require confirmation and target only the selected process."),
    ]));
  }
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
  document.getElementById("terminateBackdrop")?.remove();

  const process = state.terminateModalProcess;
  if (!process) {
    return;
  }

  const expectedText = getTerminateConfirmationText(process);
  const backdrop = document.createElement("div");
  backdrop.id = "terminateBackdrop";
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

function renderResetSettingsModal() {
  document.querySelector(".reset-modal-backdrop")?.remove();

  if (!state.resetSettingsModalOpen) {
    return;
  }

  const backdrop = document.createElement("div");
  backdrop.className = "modal-backdrop reset-modal-backdrop";

  const modal = document.createElement("div");
  modal.className = "confirm-modal reset-modal";
  modal.setAttribute("role", "dialog");
  modal.setAttribute("aria-modal", "true");
  modal.setAttribute("aria-label", "Reset settings");

  const title = document.createElement("h2");
  title.textContent = "Reset settings?";
  modal.appendChild(title);

  const text = document.createElement("p");
  text.textContent = "This clears local WPCC interface preferences and restores the default Dashboard start screen, normal process table spacing, visible executable path column, visible safety notes, normal visual effects, and update checking defaults.";
  modal.appendChild(text);

  const actions = document.createElement("div");
  actions.className = "modal-actions";

  const cancelButton = document.createElement("button");
  cancelButton.type = "button";
  cancelButton.className = "secondary-button";
  cancelButton.textContent = "Cancel";
  cancelButton.addEventListener("click", () => {
    state.resetSettingsModalOpen = false;
    render();
  });

  const resetButton = document.createElement("button");
  resetButton.type = "button";
  resetButton.className = "warning-action-button";
  resetButton.textContent = "Confirm reset";
  resetButton.addEventListener("click", () => {
    resetSettingsToDefaults();
  });

  actions.appendChild(cancelButton);
  actions.appendChild(resetButton);
  modal.appendChild(actions);
  backdrop.appendChild(modal);
  document.body.appendChild(backdrop);
  cancelButton.focus();
}

function resetSettingsToDefaults() {
  try {
    window.localStorage?.removeItem(SETTINGS_STORAGE_KEY);
    state.settingsStorageAvailable = true;
    state.settingsStorageWarning = "";
  } catch {
    state.settingsStorageAvailable = false;
    state.settingsStorageWarning = "Local settings storage is unavailable. Defaults are active for this session.";
  }

  state.settings = { ...DEFAULT_SETTINGS };
  state.resetSettingsModalOpen = false;
  render();
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

function textCell(value, className) {
  const cell = document.createElement("td");
  cell.textContent = value ?? "";
  if (className) {
    cell.className = className;
  }
  return cell;
}

function pathCell(value) {
  const cell = document.createElement("td");
  cell.className = "path-cell col-path path-column";
  cell.textContent = value;
  cell.title = value;
  return cell;
}

function badgeCell(label, tone, className) {
  const cell = document.createElement("td");
  if (className) {
    cell.className = className;
  }
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
  div.classList.add("value-line-muted");
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
  if (event.key === "Escape") {
    if (!elements.downloadCompleteModal.classList.contains("hidden-view")) {
      elements.downloadCompleteModal.classList.add("hidden-view");
      state.pendingInstallerFilePath = null;
    } else if (state.terminateModalProcess || state.freezeModalProcess || state.resetSettingsModalOpen) {
      state.terminateModalProcess = null;
      state.freezeModalProcess = null;
      state.resetSettingsModalOpen = false;
      render();
    } else if (state.editingProfileId !== null) {
      closeProfileModal();
    } else if (state.deleteModalProfile !== null) {
      closeDeleteProfileModal();
    }
  }
});

function bindUi(element, eventName, handler, debugName) {
  if (!element) {
    console.error("Missing UI element:", debugName);
    return;
  }
  element.addEventListener(eventName, handler);
}

window.chrome?.webview?.addEventListener("message", handleHostMessage);
bindUi(elements.refreshButton, "click", requestProcesses, "refreshButton");
bindUi(elements.dashboardRefreshButton, "click", requestProcesses, "dashboardRefreshButton");
bindUi(elements.quickRefreshButton, "click", requestProcesses, "quickRefreshButton");
bindUi(elements.colPriorityHeader, "click", () => handleHeaderClick("priority"), "colPriorityHeader");
bindUi(elements.colGpuHeader, "click", () => handleHeaderClick("gpu"), "colGpuHeader");
bindUi(elements.dashboardNavButton, "click", () => setActiveView("dashboard"), "dashboardNavButton");
bindUi(elements.processesNavButton, "click", () => setActiveView("processes"), "processesNavButton");
bindUi(elements.settingsNavButton, "click", () => setActiveView("settings"), "settingsNavButton");
bindUi(elements.aboutNavButton, "click", () => setActiveView("about"), "aboutNavButton");
bindUi(elements.rulesNavButton, "click", () => setActiveView("rules"), "rulesNavButton");
bindUi(elements.goToProcessesButton, "click", () => setActiveView("processes"), "goToProcessesButton");
bindUi(elements.quickProcessesButton, "click", () => setActiveView("processes"), "quickProcessesButton");
bindUi(elements.startScreenDashboard, "change", () => updateSetting("startScreen", "dashboard"), "startScreenDashboard");
bindUi(elements.startScreenProcesses, "change", () => updateSetting("startScreen", "processes"), "startScreenProcesses");
bindUi(elements.compactTableToggle, "change", (event) => updateSetting("compactProcessTable", event.target.checked), "compactTableToggle");
bindUi(elements.showPathToggle, "change", (event) => updateSetting("showExecutablePathColumn", event.target.checked), "showPathToggle");
bindUi(elements.showPidToggle, "change", (event) => updateSetting("showPidColumn", event.target.checked), "showPidToggle");
bindUi(elements.showSafetyNotesToggle, "change", (event) => updateSetting("showSafetyNotes", event.target.checked), "showSafetyNotesToggle");
bindUi(elements.reduceEffectsToggle, "change", (event) => updateSetting("reduceVisualEffects", event.target.checked), "reduceEffectsToggle");
bindUi(elements.confirmDestructiveToggle, "change", () => renderSettings(), "confirmDestructiveToggle");
bindUi(elements.updatesChecksToggle, "change", (event) => {
  const enabled = event.target.checked;
  state.settings.updateChecksEnabled = enabled;
  elements.updateIntervalSelect.disabled = !enabled;
  saveSettings();
}, "updatesChecksToggle");
bindUi(elements.updateIntervalSelect, "change", (event) => updateSetting("updateCheckInterval", event.target.value), "updateIntervalSelect");
bindUi(elements.autoRefreshSelect, "change", (event) => {
  updateSetting("autoRefreshInterval", event.target.value);
  restartAutoRefresh();
}, "autoRefreshSelect");
bindUi(elements.autoInstallToggle, "change", (event) => updateSetting("autoInstallUpdates", event.target.checked), "autoInstallToggle");
bindUi(elements.resetSettingsButton, "click", () => {
  state.resetSettingsModalOpen = true;
  render();
}, "resetSettingsButton");
bindUi(elements.manualCheckButton, "click", () => {
  checkForUpdates(true);
}, "manualCheckButton");

bindUi(elements.resetIgnoredVersionButton, "click", () => {
  state.settings.ignoredUpdateVersion = null;
  saveSettings();
  const us = loadUpdateState();
  us.ignoredVersion = null;
  saveUpdateState(us);
  renderSettings();
}, "resetIgnoredVersionButton");

bindUi(elements.dismissInstallBtn, "click", () => {
  elements.downloadCompleteModal.classList.add("hidden-view");
  state.pendingInstallerFilePath = null;
}, "dismissInstallBtn");

bindUi(elements.confirmInstallBtn, "click", () => {
  if (state.pendingInstallerFilePath) {
    postToHost({
      type: "executeInstaller",
      filePath: state.pendingInstallerFilePath,
    });
    elements.downloadCompleteModal.classList.add("hidden-view");
  }
}, "confirmInstallBtn");
bindUi(elements.searchInput, "input", (event) => {
  state.query = event.target.value;
  applyFilter();
}, "searchInput");
bindUi(elements.toggleDetailsBtn, "click", () => {
  state.detailsPanelOpen = !state.detailsPanelOpen;
  render();
}, "toggleDetailsBtn");
bindUi(elements.closeDetailsButton, "click", () => {
  state.detailsPanelOpen = false;
  render();
}, "closeDetailsButton");

function populateRunningProcessPicker() {
  const picker = elements.profileRunningProcessPicker;
  if (!picker) return;

  picker.innerHTML = "";

  const defaultOpt = document.createElement("option");
  defaultOpt.value = "";
  defaultOpt.textContent = "-- Select a running process --";
  picker.appendChild(defaultOpt);

  const excludedNames = new Set([
    "smss.exe",
    "wininit.exe",
    "spoolsv.exe",
    "searchindexer.exe",
    "rtkauduservice64.exe",
    "securityhealthservice.exe",
    "sendevsvc.exe",
    "wmiprvse.exe",
    "wudfhost.exe"
  ]);

  const uniqueProcesses = [];
  const seenPaths = new Set();
  const seenNames = new Set();

  for (const proc of state.processes) {
    if (!proc.name) continue;

    // Filter rules
    const nameLower = proc.name.toLowerCase();
    if (proc.pid <= 4) continue;
    if (proc.accessStatus === "Protected/System" || proc.accessStatus === "Access denied") continue;
    if (excludedNames.has(nameLower)) continue;

    const pathNormalized = (proc.path || "").toLowerCase().replace(/\\/g, "/");
    if (pathNormalized.includes("system32") ||
        pathNormalized.includes("systemapps") ||
        pathNormalized.includes("syswow64") ||
        pathNormalized.includes("wbem")) {
      continue;
    }

    const pathKey = proc.path ? proc.path.toLowerCase() : "";

    if (pathKey) {
      if (!seenPaths.has(pathKey)) {
        seenPaths.add(pathKey);
        uniqueProcesses.push(proc);
      }
    } else {
      if (!seenNames.has(nameLower)) {
        seenNames.add(nameLower);
        uniqueProcesses.push(proc);
      }
    }
  }

  uniqueProcesses.sort((a, b) => {
    const nameA = a.name.toLowerCase();
    const nameB = b.name.toLowerCase();
    return nameA.localeCompare(nameB);
  });

  for (const proc of uniqueProcesses) {
    const opt = document.createElement("option");
    opt.value = JSON.stringify({ path: proc.path || "", name: proc.name || "" });
    opt.textContent = proc.name + (proc.path ? ` (${proc.path})` : "");
    picker.appendChild(opt);
  }
}

function updateHeaderIndicators() {
  const priorityTh = elements.colPriorityHeader;
  const gpuTh = elements.colGpuHeader;
  if (!priorityTh || !gpuTh) return;

  priorityTh.textContent = "CPU Priority";
  gpuTh.textContent = "GPU Preference";

  if (state.sortColumn === "priority") {
    if (state.sortDirection === "asc") {
      priorityTh.textContent = "CPU Priority ▲";
    } else if (state.sortDirection === "desc") {
      priorityTh.textContent = "CPU Priority ▼";
    }
  } else if (state.sortColumn === "gpu") {
    if (state.sortDirection === "asc") {
      gpuTh.textContent = "GPU Preference ▲";
    } else if (state.sortDirection === "desc") {
      gpuTh.textContent = "GPU Preference ▼";
    }
  }
}

function handleHeaderClick(column) {
  if (state.sortColumn !== column) {
    state.sortColumn = column;
    state.sortDirection = "asc";
  } else {
    if (state.sortDirection === "asc") {
      state.sortDirection = "desc";
    } else if (state.sortDirection === "desc") {
      state.sortDirection = "none";
      state.sortColumn = null;
    } else {
      state.sortDirection = "asc";
    }
  }
  applyFilter();
}

function mapPriorityToValue(priority) {
  const norm = normalizePriority(priority);
  switch (norm) {
    case "Realtime": return 6;
    case "High": return 5;
    case "Above Normal": return 4;
    case "Normal": return 3;
    case "Below Normal": return 2;
    case "Idle": return 1;
    default: return 0;
  }
}

function mapGpuToValue(preference) {
  switch (preference) {
    case "HighPerformance": return 3;
    case "PowerSaving": return 2;
    case "SystemDefault": return 1;
    default: return 0;
  }
}

function ensureProfileModalDom() {
  if (document.getElementById("profileModal")) return;

  const deleteModal = document.getElementById("deleteProfileModal");
  const profilesList = document.getElementById("profilesList");

  const html = `<div id="profileModal" class="modal-backdrop hidden-view">
    <div class="confirm-modal profile-form-modal">
      <h2 id="profileModalTitle">Create profile</h2>
      
      <form id="profileForm">
        <div class="form-group">
          <label for="profileName">Profile name</label>
          <input type="text" id="profileName" placeholder="e.g. Heavy Game Preset" required>
        </div>

        <div class="form-group">
          <label for="profileRunningProcessPicker">Select running process</label>
          <select id="profileRunningProcessPicker" class="setting-select">
            <option value="">-- Select a running process --</option>
          </select>
        </div>

        <div class="form-row-flex">
          <div class="form-group">
            <label for="profileMatchMode">Match mode</label>
            <select id="profileMatchMode" class="setting-select">
              <option value="path">Executable path</option>
              <option value="name">Process name</option>
            </select>
          </div>
        </div>

        <div class="form-group" id="exePathGroup">
          <label for="profileExePath">Target executable path (preferred)</label>
          <input type="text" id="profileExePath" placeholder="e.g. C:\\Games\\HeavyGame.exe">
        </div>

        <div class="form-group" id="processNameGroup">
          <label for="profileProcessName">Target process name (fallback)</label>
          <input type="text" id="profileProcessName" placeholder="e.g. HeavyGame.exe">
        </div>

        <div class="form-row-double">
          <div class="form-group">
            <label for="profileCpuPriority">CPU Priority</label>
            <select id="profileCpuPriority" class="setting-select">
              <option value="DoNotChange">Do not change</option>
              <option value="High">High</option>
              <option value="AboveNormal">Above normal</option>
              <option value="Normal">Normal</option>
              <option value="BelowNormal">Below normal</option>
              <option value="Idle">Idle</option>
              <option value="Realtime">Realtime</option>
            </select>
          </div>

          <div class="form-group">
            <label for="profileGpuPreference">GPU Preference</label>
            <select id="profileGpuPreference" class="setting-select">
              <option value="DoNotChange">Do not change</option>
              <option value="SystemDefault">System default</option>
              <option value="PowerSaving">Power saving / iGPU</option>
              <option value="HighPerformance">High performance / dGPU</option>
            </select>
          </div>
        </div>

        <div id="profileRealtimeWarningBlock" class="modal-warning hidden-view">
          <p>Warning: Realtime priority can make Windows less responsive.</p>
          <label class="profile-form-checkbox-label">
            <input type="checkbox" id="profileRealtimeCheckbox">
            <span class="profile-form-warning-span">I understand the risk of Realtime priority in this profile.</span>
          </label>
        </div>

        <div class="form-switches">
          <div class="switch-item">
            <div class="switch-label-group">
              <strong>Apply to family</strong>
              <span>Applies to all instances sharing this app target</span>
            </div>
            <label class="switch-control">
              <input id="profileApplyToFamily" type="checkbox">
              <span class="switch-track"><span class="switch-thumb"></span></span>
            </label>
          </div>

          <div class="switch-item locked-item">
            <div class="switch-label-group">
              <strong>Auto apply</strong>
              <span>Planned / Inactive — Not available yet</span>
            </div>
            <label class="switch-control locked">
              <input id="profileAutoApply" type="checkbox" disabled>
              <span class="switch-track"><span class="switch-thumb"></span></span>
            </label>
          </div>
        </div>

        <div class="form-group">
          <label for="profileNotes">Notes</label>
          <textarea id="profileNotes" rows="2" placeholder="Enter optional notes about this profile..."></textarea>
        </div>

        <div class="modal-actions">
          <button id="profileDeleteButton" class="danger-action-button hidden" type="button">Delete profile</button>
          <button id="profileResetButton" class="secondary-button" type="button">Reset form</button>
          <button id="profileCancelButton" class="secondary-button" type="button">Cancel</button>
          <button id="profileSaveButton" class="primary-button" type="submit">Save profile</button>
        </div>
      </form>
    </div>
  </div>`;

  if (deleteModal) {
    deleteModal.insertAdjacentHTML("beforebegin", html);
  } else if (profilesList) {
    profilesList.insertAdjacentHTML("afterend", html);
  } else {
    return;
  }

  elements.profileModal = document.getElementById("profileModal");
  elements.profileModalTitle = document.getElementById("profileModalTitle");
  elements.profileForm = document.getElementById("profileForm");
  elements.profileName = document.getElementById("profileName");
  elements.profileRunningProcessPicker = document.getElementById("profileRunningProcessPicker");
  elements.profileMatchMode = document.getElementById("profileMatchMode");
  elements.profileExePath = document.getElementById("profileExePath");
  elements.profileProcessName = document.getElementById("profileProcessName");
  elements.exePathGroup = document.getElementById("exePathGroup");
  elements.processNameGroup = document.getElementById("processNameGroup");
  elements.profileCpuPriority = document.getElementById("profileCpuPriority");
  elements.profileGpuPreference = document.getElementById("profileGpuPreference");
  elements.profileRealtimeWarningBlock = document.getElementById("profileRealtimeWarningBlock");
  elements.profileRealtimeCheckbox = document.getElementById("profileRealtimeCheckbox");
  elements.profileApplyToFamily = document.getElementById("profileApplyToFamily");
  elements.profileAutoApply = document.getElementById("profileAutoApply");
  elements.profileNotes = document.getElementById("profileNotes");
  elements.profileResetButton = document.getElementById("profileResetButton");
  elements.profileCancelButton = document.getElementById("profileCancelButton");
  elements.profileSaveButton = document.getElementById("profileSaveButton");
  elements.profileDeleteButton = document.getElementById("profileDeleteButton");
}

// Bind Profile v1 UI Events
ensureProfileModalDom();
bindUi(elements.createProfileButton, "click", () => openProfileModal(null), "createProfileButton");
bindUi(elements.emptyCreateProfileButton, "click", () => openProfileModal(null), "emptyCreateProfileButton");
bindUi(elements.profileCancelButton, "click", closeProfileModal, "profileCancelButton");
bindUi(elements.profileResetButton, "click", resetProfileForm, "profileResetButton");
bindUi(elements.profileRunningProcessPicker, "change", (event) => {
  const val = event.target.value;
  if (!val) return;
  try {
    const data = JSON.parse(val);
    elements.profileExePath.value = data.path || "";
    elements.profileProcessName.value = data.name || "";
    if (!elements.profileName.value || elements.profileName.value === "Unnamed Profile") {
      elements.profileName.value = `${data.name} Preset`;
    }
  } catch (e) {
    console.error("Failed to parse process picker value", e);
  }
}, "profileRunningProcessPicker");
bindUi(elements.profileMatchMode, "change", updateMatchModeUi, "profileMatchMode");
bindUi(elements.profileCpuPriority, "change", updateCpuRealtimeUi, "profileCpuPriority");
bindUi(elements.profileRealtimeCheckbox, "change", updateCpuRealtimeUi, "profileRealtimeCheckbox");
bindUi(elements.profileForm, "submit", saveProfileForm, "profileForm");
bindUi(elements.profileDeleteButton, "click", () => {
  if (state.editingProfileId) {
    const prof = state.profilesState.profiles.find(p => p.id === state.editingProfileId);
    if (prof) {
      closeProfileModal();
      openDeleteProfileModal(prof);
    }
  }
}, "profileDeleteButton");
bindUi(elements.deleteProfileCancelButton, "click", closeDeleteProfileModal, "deleteProfileCancelButton");
bindUi(elements.deleteProfileConfirmButton, "click", confirmDeleteProfile, "deleteProfileConfirmButton");
bindUi(elements.exportProfilesButton, "click", exportProfiles, "exportProfilesButton");
bindUi(elements.importProfilesButton, "click", importProfiles, "importProfilesButton");
bindUi(elements.importFileInput, "change", handleImportFile, "importFileInput");
bindUi(elements.searchInput, "input", (e) => {
  state.query = e.target.value;
  applyFilter();
}, "searchInput");

document.querySelectorAll("th.clickable-header").forEach(th => {
  th.addEventListener("click", () => {
    const col = th.dataset.sort;
    if (!col) return;
    if (state.sortColumn === col) {
      if (state.sortDirection === "asc") state.sortDirection = "desc";
      else if (state.sortDirection === "desc") { state.sortDirection = "none"; state.sortColumn = null; }
      else state.sortDirection = "asc";
    } else {
      state.sortColumn = col;
      state.sortDirection = "asc";
    }
    applyFilter();
  });
});

render();
requestProcesses();
requestNativeProfiles();
restartAutoRefresh();
runAutoUpdateCheckIfNeeded();

