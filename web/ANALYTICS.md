# Website analytics

The Salt installer and Studio use GA4 measurement ID `G-JVF09MZEGD`.
Only `chancethemaker.github.io/Felucca/` and `chancethemaker.github.io/felucca-smk/` send analytics. Local previews,
other forks, and upstream sites do not. Analytics cookies are enabled by default.
A bottom notice offers Keep analytics, Essential only, and Cookie settings.
Essential only disables analytics; the settings menu can reopen the choice.
Existing declines remain full opt-outs, and a previously saved cookieless choice
is preserved until changed. All choices leave every feature available.
Preferences persist in local storage and can be changed at any time.

Advertising consent and Google signals remain disabled. Page URLs omit
query strings; referrers are reduced to their origin. Custom event fields
are restricted to firmware version, stage, fixed error code, file type, and the
current theme's allowlisted identifier. Page views also carry `theme`.
Do not add MIDI messages, device identifiers, preset names, or raw errors.

| Event | Meaning |
| --- | --- |
| `install_attempt` | Install clicked, before MIDI permission |
| `install_write_started` | First write progress callback, once per attempt |
| `install_success` | Normal install returned and confirmed expected device identity |
| `install_failed` | Permission, discovery, transfer, or verification failure |
| `install_resume_complete` | Recovery write finished; reboot identity is not verified by the resume path |
| `browser_audio_started` | Browser synth start resolved |
| `download_click` | Click on a ZIP or FWSC link; not proof of completed download |
| `theme_changed` | User selects a website theme in the settings menu |

Automatic package fetching is not a download click. A retry is a new attempt.
Counts are approximate: consent, blockers, lost connections, and closed tabs
can prevent reporting. No analytics call is awaited by an installation.

In GA4, mark `install_success` as a key event if desired. Register
`theme`, `firmware_version`, `stage`, `error_code`, and `file_type` as event-scoped
custom dimensions to break down these events in reports. Review enhanced
measurement in the web stream; these explicit custom events are separate
from Google's automatic download events. The `theme` dimension uses event
parameter `theme` (for example, `space`, `matrix`, or `stage`). Selecting a gallery
thumbnail opens Studio with that theme; its page view contains that theme.
Theme names are independent of the UI language. Verify visits in Realtime;
local unit tests never contact Google.

Run `node web/test_analytics.mjs` for consent, environment, and filtering checks.
