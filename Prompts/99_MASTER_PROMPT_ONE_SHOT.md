
# Master prompt — jeden duży prompt do Codexa

Użyj tego tylko wtedy, jeśli chcesz dać Codexowi cały projekt naraz.  
Moja opinia: lepiej używać tasków po kolei, ale ten prompt może być dobry jako ogólny brief projektu.

---

Stwórz od zera aplikację desktopową **Windows Process Control Center** w **C++20** dla **Windows 10/11 x64**.

Program ma służyć do zarządzania aktywnymi procesami w Windowsie. Ma mieć nowoczesne, czytelne GUI bez klasycznego wyglądu Windows XP. Użyj **Win32 + DirectX 11 + Dear ImGui**. Projekt ma budować się przez **CMake + MSVC / Visual Studio 2022**.

## Funkcje główne

Aplikacja ma umożliwiać:

1. Wyświetlanie aktywnych procesów:
   - nazwa,
   - PID,
   - parent PID,
   - ścieżka exe,
   - RAM,
   - priorytet CPU,
   - status,
   - session ID,
   - user/system/inaccessible/protected classification.

2. Zakończenie procesu:
   - `TerminateProcess`,
   - potwierdzenie użytkownika,
   - blokada dla procesów krytycznych, systemowych i własnego procesu aplikacji,
   - czytelny log wyniku.

3. Zamrożenie i wznowienie procesu:
   - przez enumerację wątków i `SuspendThread/ResumeThread`,
   - bez undocumented `NtSuspendProcess` w MVP,
   - tracking procesów zamrożonych przez tę aplikację,
   - blokady bezpieczeństwa.

4. Ustawianie priorytetu CPU:
   - Idle,
   - Below normal,
   - Normal,
   - Above normal,
   - High,
   - Realtime.
   - Realtime wymaga mocnego ostrzeżenia i osobnego potwierdzenia.

5. Ustawianie preferencji GPU:
   - Let Windows decide,
   - Power saving / iGPU,
   - High performance / dedicated GPU, np. RTX.
   - Realizuj przez `HKCU\Software\Microsoft\DirectX\UserGpuPreferences`.
   - Zapisuj preferencję dla ścieżki `.exe`.
   - UI musi jasno mówić, że zmiana może wymagać restartu docelowej aplikacji i nie jest gwarantowanym przełączeniem działającego procesu w locie.
   - Wykrywanie adapterów przez DXGI pokazuj tylko informacyjnie.

## UI / UX

GUI ma być:

- ciemne,
- czyste,
- nowoczesne,
- bez przeładowania,
- dobrze przeskalowane,
- działające przy DPI 100%, 125%, 150%,
- czytelne na 1280x800,
- z tabelą procesów, prawym panelem szczegółów i dolnym logiem,
- z wyszukiwarką, filtrami, sortowaniem i auto-refresh,
- bez nachodzenia elementów.

## Bezpieczeństwo

Nie dodawaj:

- malware,
- stealth,
- ukrywania procesów,
- injection,
- driverów kernelowych,
- obchodzenia zabezpieczeń Windows,
- automatycznego zabijania procesów bez użytkownika,
- telemetrii.

Dodaj:

- blokady dla procesów krytycznych,
- komunikaty `Access Denied`,
- wykrywanie admin/user mode,
- przycisk `Restart as administrator`,
- log operacji,
- dokumentację ograniczeń.

## Struktura

Utwórz sensowną strukturę:

```text
WindowsProcessControlCenter/
  CMakeLists.txt
  README.md
  docs/
    ARCHITECTURE.md
    PROGRESS.md
    SAFETY_NOTES.md
    MANUAL_TEST_CHECKLIST.md
  scripts/
    build.ps1
    clean.ps1
  src/
    main.cpp
    app/
    core/
    ui/
    platform/
  tests/
```

## Wymagania jakościowe

Po każdym większym etapie:

- uruchom build Debug i Release,
- napraw błędy,
- zaktualizuj `docs/PROGRESS.md`,
- nie zostawiaj TODO jako zamiennika implementacji,
- nie udawaj, że funkcja działa, jeśli Windows jej nie wspiera.

## Definicja ukończenia

Projekt jest gotowy, gdy:

- Release build działa,
- aplikacja uruchamia się bez crasha,
- procesy są widoczne,
- terminate działa na zwykłym procesie użytkownika,
- priority change działa na zwykłym procesie użytkownika,
- freeze/resume działa na `notepad.exe`,
- GPU preference zapisuje się dla ścieżki exe,
- UI jest czytelne i nic nie nachodzi,
- dokumentacja jasno opisuje ograniczenia.
