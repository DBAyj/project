# 15 Git Convention

`main` is the stable baseline and `develop` is the current integration branch. Work branches are `feature/*`, `fix/*`, `docs/*`, `release/*`, and `hotfix/*`.

Commit prefixes are `feat:`, `fix:`, `docs:`, `refactor:`, `test:`, `build:`, `ci:`, `perf:`, `security:`, and `chore:`. Examples: `docs: freeze AstraOS system architecture`, `feat: add projection session protocol`, and `security: define capability permission model`. Non-descriptive messages such as `update`, `修改一下`, `测试`, `最终版`, and `123` are prohibited.

Every material frozen-design change must reference an ADR. A commit must include validation evidence relevant to its changed contracts.
