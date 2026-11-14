# Note for Stew: letting Grok see the shared rules (2026-11-14)

From Ron. A suggestion only; AGENTS.md is your file, so it's your call.

## What I noticed

Since 11-13 the rules we both follow are in `CLAUDE.md` at the repo root: pull before starting,
the "finished" checklist (installers, Addendum, one commit, GitHub Release), keeping the Pi,
Windows and Debian trees in step, and version numbers only in the Software Versions Addendum.

Claude reads that file. Grok reads `AGENTS.md` instead, and nothing in `AGENTS.md` points to
`CLAUDE.md`, so your Grok may not know those rules. `AGENTS.md` was also last changed on 10-07,
so its "Current work" and "Current kits" lines are from early October.

## Suggestion

Add these lines near the top of `AGENTS.md` (right after the "On start" line):

    **Shared rules:** read `CLAUDE.md` at the repo root as well. It holds the rules Ron and Stew
    both follow (pull first, the "finished" checklist, trees in step, versions in the Addendum).
    If `AGENTS.md` and `CLAUDE.md` disagree, `CLAUDE.md` wins.

Then both of our tools follow the same rule book, and a rule only has to be written down once.

## Optional

- Update or remove the "Current work" and "Current kits" lines in `AGENTS.md`, since they are
  from 10-07. The current versions are always in
  `docs/manuals/MSCC-Software-Versions-Addendum.md`.
- If your Grok work orders (cmd-078 to cmd-080) are useful for me to see, push them with your
  next commit; the repo copy of `.mscc-coord` stops at cmd-077.
