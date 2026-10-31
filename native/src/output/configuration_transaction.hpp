#pragma once

namespace zenith {
enum class ConfigurationResult { Rejected, Tested, Applied, Restored, RecoveryRequired };

// Keep test and apply sequencing identical. Publishing/reconciliation belongs to
// the caller, after this transaction finishes, never between individual heads.
template<class Test, class Prepare, class Commit, class Restore>
ConfigurationResult run_configuration_transaction(
	bool test_only,
	Test test,
	Prepare prepare,
	Commit commit,
	Restore restore
) {
	if (!test() || !prepare())
		return ConfigurationResult::Rejected;
	if (test_only)
		return ConfigurationResult::Tested;
	if (commit())
		return ConfigurationResult::Applied;
	return restore() ? ConfigurationResult::Restored : ConfigurationResult::RecoveryRequired;
}
}
