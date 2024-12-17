
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Experiment.h"
#include "winget/UserSettings.h"
#include "Experiment/Experiment.h"
#include "AppInstallerTelemetry.h"

namespace AppInstaller::Settings
{
    using namespace Experiment;

    namespace
    {
        ExperimentState GetExperimentStateInternal(ExperimentKey key, const UserSettings& userSettings)
        {
            if (!GroupPolicies().IsEnabled(TogglePolicy::Policy::Experiments))
            {
                AICLI_LOG(Core, Info, << "Experiment " << Experiment::GetExperiment(key).Name() <<
                    " is disabled due to group policy: " << TogglePolicy::GetPolicy(TogglePolicy::Policy::Experiments).RegValueName());
                return { false, ExperimentToggleSource::Policy };
            }

            if (key == ExperimentKey::None)
            {
                return { false, ExperimentToggleSource::Default };
            }

            auto experiment = Experiment::GetExperiment(key);
            auto userSettingsExperiments = userSettings.Get<Setting::Experiments>();
            auto experimentJsonName = experiment.JsonName();
            if (userSettingsExperiments.find(experimentJsonName) != userSettingsExperiments.end())
            {
                auto isEnabled = userSettingsExperiments[experimentJsonName];
                AICLI_LOG(Core, Info, << "Experiment " << experiment.Name() << " is set to " << (isEnabled ? "true" : "false") << " in user settings");
                return { isEnabled, ExperimentToggleSource::UserSetting };
            }

            auto isEnabled = AppInstaller::Experiment::IsEnabled(experiment.GetKey());
            AICLI_LOG(Core, Info, << "Experiment " << experiment.Name() << " is set to " << (isEnabled ? "true" : "false"));
            return { isEnabled, ExperimentToggleSource::Default };
        }

        std::string ExperimentToggleSourceToString(ExperimentToggleSource source)
        {
            switch (source)
            {
            case ExperimentToggleSource::Default:
                return "Default";
            case ExperimentToggleSource::Policy:
                return "Policy";
            case ExperimentToggleSource::UserSetting:
                return "UserSetting";
            default:
                return "Unknown";
            }
        }
    }

    std::string ExperimentState::ToJson() const
    {
        Json::Value root;
        root["IsEnabled"] = m_isEnabled;
        root["ToggleSource"] = ExperimentToggleSourceToString(m_toggleSource);
        Json::StreamWriterBuilder builder;
        return Json::writeString(builder, root);
    }

    ExperimentState Experiment::GetStateInternal(ExperimentKey key)
    {
        return GetExperimentStateInternal(key, User());
    }

    ExperimentState Experiment::GetState(ExperimentKey key)
    {
        return Logging::Telemetry().GetExperimentState(key);
    }

    Experiment Experiment::GetExperiment(ExperimentKey key)
    {
        switch (key)
        {
        case ExperimentKey::CDN:
            return Experiment{ "winget source CDN experiment", "CDN", "https://aka.ms/winget-settings", ExperimentKey::CDN};
#ifndef AICLI_DISABLE_TEST_HOOKS
        case ExperimentKey::TestExperiment:
            return Experiment{ "Test experiment", "TestExperiment", "https://aka.ms/winget-settings", ExperimentKey::TestExperiment };
#endif
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    std::vector<Experiment> Experiment::GetAllExperiments()
    {
        std::vector<Experiment> result;

        for (Key_t i = 0x1; i < static_cast<Key_t>(ExperimentKey::Max); ++i)
        {
            result.emplace_back(GetExperiment(static_cast<ExperimentKey>(i)));
        }

        return result;
    }
}
