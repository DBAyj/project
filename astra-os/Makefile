.PHONY: docs-verify format lint build test

docs-verify:
	python3 scripts/verify_document_baseline.py

format:
	python3 scripts/check_p1_format.py

lint:
	shellcheck scripts/*.sh
	python3 -m py_compile scripts/*.py tests/contract/test_p1_schemas.py tests/contract/test_p2_schemas.py tests/contract/test_p3_schemas.py tests/contract/test_p4_schemas.py tests/contract/test_p5_schemas.py tests/p1-e2e/test_p1_graphics_backend.py tests/p2-e2e/test_p2_live_service.py tests/p3-e2e/test_p3_live_service.py tests/p4-e2e/test_p4_visual.py
	python3.12 -m compileall -q services/astra-intent-service/src services/astra-intent-service/tests tests/contract/test_p2_schemas.py

build: p1-build p2-build p3-build p4-build p5-build

test: p1-test p2-test p3-test p4-test p5-test

.PHONY: p1-configure p1-build p1-run p1-stop p1-test p1-verify p1-clean \
	p1-test-policy p1-test-error-registry p1-test-audit-schema p1-test-config-runtime \
	p1-test-accessibility p1-test-metal p1-test-window-interaction p1-release-gate
p1-configure:
	./scripts/configure_p1.sh
p1-build:
	./scripts/build_p1.sh
p1-run:
	./scripts/run_p1.sh
p1-stop:
	./scripts/stop_p1.sh
p1-test:
	./scripts/test_p1.sh
p1-verify:
	./scripts/verify_p1.sh
p1-clean:
	./scripts/clean_p1_build.sh

p1-test-policy:
	./scripts/build_p1.sh
	ctest --test-dir runtime/tmp/p1-build -R '^astra-shell-projection-policy-tests$$' --output-on-failure

p1-test-error-registry:
	./scripts/build_p1.sh
	ctest --test-dir runtime/tmp/p1-build -R '^astra-error-code-registry-tests$$' --output-on-failure

p1-test-audit-schema:
	./scripts/test_p1_audit_schema.sh

p1-test-config-runtime:
	./scripts/build_p1.sh
	ctest --test-dir runtime/tmp/p1-build -R '^astra-shell-config-tests$$' --output-on-failure
	QT_QPA_PLATFORM=offscreen QT_QUICK_CONTROLS_STYLE=Basic "$$($(shell command -v brew) --prefix qt)/bin/qmltestrunner" -input apps/astra-shell/tests/qml -silent

p1-test-accessibility:
	./scripts/test_p1_accessibility.sh

p1-test-metal:
	./scripts/test_p1_metal.sh

p1-test-window-interaction:
	./scripts/test_p1_window_interaction.sh

p1-release-gate:
	$(MAKE) format
	$(MAKE) lint
	$(MAKE) p1-configure
	$(MAKE) p1-build
	$(MAKE) p1-test
	$(MAKE) p1-test-policy
	$(MAKE) p1-test-error-registry
	$(MAKE) p1-test-audit-schema
	$(MAKE) p1-test-config-runtime
	$(MAKE) p1-test-accessibility
	$(MAKE) p1-test-metal
	$(MAKE) p1-test-window-interaction
	$(MAKE) p1-verify
	$(MAKE) docs-verify

.PHONY: p2-configure p2-build p2-run p2-stop p2-test p2-test-unit p2-test-contract \
	p2-test-integration p2-test-security p2-test-performance p2-verify p2-clean
p2-configure:
	./scripts/configure_p2.sh
p2-build:
	./scripts/build_p2.sh
p2-run:
	./scripts/run_p2.sh
p2-stop:
	./scripts/stop_p2.sh
p2-test:
	./scripts/test_p2.sh
p2-test-unit:
	./scripts/test_p2.sh unit
p2-test-contract:
	./scripts/test_p2.sh contract
p2-test-integration:
	./scripts/test_p2.sh integration
p2-test-security:
	./scripts/test_p2.sh security
p2-test-performance:
	./scripts/test_p2.sh performance
p2-verify:
	./scripts/verify_p2.sh
p2-clean:
	./scripts/clean_p2_build.sh

.PHONY: p3-configure p3-build p3-run p3-stop p3-test p3-verify p3-clean
p3-configure:
	./scripts/configure_p3.sh
p3-build:
	./scripts/build_p3.sh
p3-run:
	./scripts/run_p3.sh
p3-stop:
	./scripts/stop_p3.sh
p3-test:
	./scripts/test_p3.sh
p3-verify:
	./scripts/verify_p3.sh
p3-clean:
	./scripts/clean_p3_build.sh

.PHONY: p4-configure p4-build p4-run p4-stop p4-test p4-test-unit p4-test-contract \
	p4-test-graphics p4-test-visual p4-test-integration p4-test-security p4-test-performance \
	p4-test-stability p4-generate-fixtures p4-verify p4-release-gate p4-clean
p4-configure:
	./scripts/configure_p4.sh
p4-build:
	./scripts/build_p4.sh
p4-run:
	./scripts/run_p4.sh
p4-stop:
	./scripts/stop_p4.sh
p4-test:
	./scripts/test_p4.sh
p4-test-unit:
	./scripts/build_p4.sh
	ctest --test-dir runtime/tmp/p4-build -R '^(astra-render|astra-projection|astra-shell-p4-projection-integration|astra-shell-projection-service-client)-' --output-on-failure
p4-test-contract:
	./.venv/bin/python tests/contract/test_p4_schemas.py
p4-test-graphics:
	./scripts/test_p4_graphics.sh
p4-test-visual:
	./scripts/build_p4.sh
	./scripts/generate_p4_fixtures.sh
	python3 tools/projection-fixture-generator/test_generate.py
	python3 tests/p4-e2e/test_p4_visual.py
	ctest --test-dir runtime/tmp/p4-build -R '^(astra-projection-service-tests|astra-shell-p4-projection-integration-tests)$$' --output-on-failure
p4-test-integration:
	./scripts/test_p4_integration.sh
p4-test-security:
	./scripts/test_p4_security.sh
p4-test-performance:
	./scripts/test_p4_performance.sh
p4-test-stability:
	./scripts/test_p4_stability.sh
p4-generate-fixtures:
	./scripts/generate_p4_fixtures.sh
p4-verify:
	./scripts/verify_p4.sh
p4-release-gate:
	./scripts/release_gate_p4.sh
p4-clean:
	./scripts/clean_p4_build.sh

.PHONY: p5-configure p5-build p5-run p5-stop p5-test p5-test-unit p5-test-contract \
	p5-test-qml p5-test-integration p5-test-accessibility p5-test-graphics \
	p5-test-security p5-test-performance p5-test-stability p5-generate-fixtures \
	p5-verify p5-release-gate p5-clean
p5-configure:
	./scripts/configure_p5.sh
p5-build:
	./scripts/build_p5.sh
p5-run:
	./scripts/run_p5.sh
p5-stop:
	./scripts/stop_p5.sh
p5-test:
	./scripts/test_p5.sh
p5-test-unit:
	./scripts/test_p5_unit.sh
p5-test-contract:
	./scripts/test_p5_contract.sh
p5-test-qml:
	./scripts/test_p5_qml.sh
p5-test-integration:
	./scripts/test_p5_integration.sh
p5-test-accessibility:
	./scripts/test_p5_accessibility.sh
p5-test-graphics:
	./scripts/test_p5_graphics.sh
p5-test-security:
	./scripts/test_p5_security.sh
p5-test-performance:
	./scripts/test_p5_performance.sh
p5-test-stability:
	./scripts/test_p5_stability.sh
p5-generate-fixtures:
	./scripts/generate_spatial_ui_fixtures.sh
p5-verify:
	./scripts/verify_p5.sh
p5-release-gate:
	./scripts/release_gate_p5.sh
p5-clean:
	./scripts/clean_p5_build.sh
