
# README — folder promptów dla Codexa

Ten folder zawiera gotowe prompty `.md`, które możesz wrzucać Codexowi po kolei.  
Cel: zbudować od zera program C++ do zarządzania aktywnymi procesami w Windowsie.

## Kolejność pracy

1. `01_PROJECT_BOOTSTRAP.md`
2. `02_PROCESS_ENUMERATION.md`
3. `03_SAFE_PROCESS_ACTIONS.md`
4. `04_CPU_PRIORITY_MANAGEMENT.md`
5. `05_FREEZE_AND_RESUME_PROCESSES.md`
6. `06_GPU_PREFERENCE_MANAGEMENT.md`
7. `07_MODERN_GUI_AND_LAYOUT.md`
8. `08_ELEVATION_SAFETY_AND_GUARDS.md`
9. `09_SETTINGS_LOGS_AND_PERSISTENCE.md`
10. `10_TESTING_PACKAGING_AND_RELEASE.md`
11. `11_FINAL_POLISH_AND_QA.md`

## Moja opinia

Najrozsądniej robić ten projekt etapami, a nie jednym gigantycznym promptem.  
To jest aplikacja dotykająca WinAPI, procesów, uprawnień i rejestru, więc Codex powinien mieć jasne checkpointy, build po każdym tasku i twarde ograniczenia bezpieczeństwa.

## Uwaga o GPU

Nie zlecaj Codexowi „przełączania działającego procesu z iGPU na RTX w locie”, bo Windows zwykle tak tego nie obsługuje globalnie i niezawodnie.  
W tym projekcie funkcja GPU ma ustawiać preferencję Windows dla konkretnej ścieżki `.exe`:

- `Let Windows decide`
- `Power saving / iGPU`
- `High performance / dGPU`

UI ma jasno mówić: **„może wymagać ponownego uruchomienia procesu/aplikacji”**.
