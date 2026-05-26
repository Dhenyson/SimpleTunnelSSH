#include <QCoreApplication>

#include <iostream>

#include "TestRegistry.h"

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);

    try
    {
        SimpleTunnelSSH::Core::Tests::runPortConflictDetectorTests();
        SimpleTunnelSSH::Core::Tests::runSshCommandBuilderTests();
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';
        return 1;
    }

    return 0;
}