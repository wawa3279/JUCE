/*
  ==============================================================================

   This file is part of the JUCE 8 technical preview.
   Copyright (c) Raw Material Software Limited

   You may use this code under the terms of the GPL v3
   (see www.gnu.org/licenses).

   For the technical preview this file cannot be licensed commercially.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <JuceHeader.h>

//==============================================================================
class ConsoleLogger final : public Logger
{
    void logMessage (const String& message) override
    {
        std::cout << message << std::endl;

       #if JUCE_WINDOWS
        Logger::outputDebugString (message);
       #endif
    }
};

//==============================================================================
class ConsoleUnitTestRunner final : public UnitTestRunner
{
    void logMessage (const String& message) override
    {
        Logger::writeToLog (message);
    }
};


//==============================================================================
int main (int argc, char **argv)
{
    ArgumentList args (argc, argv);

    if (args.containsOption ("--help|-h"))
    {
        std::cout << argv[0] << " [--help|-h] [--list-categories] [--category=category] [--seed=seed]" << std::endl;
        return 0;
    }

    if (args.containsOption ("--list-categories"))
    {
        for (auto& category : UnitTest::getAllCategories())
            std::cout << category << std::endl;

        return  0;
    }

    ConsoleLogger logger;
    Logger::setCurrentLogger (&logger);

    ConsoleUnitTestRunner runner;

    auto seed = [&args]
    {
        if (args.containsOption ("--seed"))
        {
            auto seedValueString = args.getValueForOption ("--seed");

            if (seedValueString.startsWith ("0x"))
                return seedValueString.getHexValue64();

            return seedValueString.getLargeIntValue();
        }

        return Random::getSystemRandom().nextInt64();
    }();

    if (args.containsOption ("--category"))
        runner.runTestsInCategory (args.getValueForOption ("--category"), seed);
    else
        runner.runAllTests (seed);

    std::vector<String> failures;

    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        auto* result = runner.getResult (i);

        if (result->failures > 0)
            failures.push_back (result->unitTestName + " / " + result->subcategoryName + ": " + String (result->failures) + " test failure" + (result->failures > 1 ? "s" : ""));
    }

    if (! failures.empty())
    {
        logger.writeToLog (newLine + "Test failure summary:" + newLine);

        for (const auto& failure : failures)
            logger.writeToLog (failure);

        Logger::setCurrentLogger (nullptr);
        return 1;
    }

    logger.writeToLog (newLine + "All tests completed successfully");
    Logger::setCurrentLogger (nullptr);

    return 0;
}
