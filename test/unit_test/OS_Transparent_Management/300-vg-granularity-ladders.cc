#include <catch.hpp>

#include "ProjectConfiguration.h" // User file

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE) && (IDEAL_VARIABLE_GRANULARITY == ENABLE)
#include "OS_Transparent_Management/os_transparent_management.h"

namespace
{
constexpr START_ADDRESS_WIDTH last_data_line = START_ADDRESS_WIDTH(OS_TRANSPARENT_MANAGEMENT::StartAddress::Max) - 1;

/** @brief The smallest power of two strictly greater than distance */
MIGRATION_GRANULARITY_WIDTH next_power_of_two_above(unsigned distance)
{
    MIGRATION_GRANULARITY_WIDTH granularity = 1;
    while (granularity <= distance)
    {
        granularity = static_cast<MIGRATION_GRANULARITY_WIDTH>(granularity * 2);
    }

    return granularity;
}
} // namespace

SCENARIO("Variable granularity rounds a migration up to a whole migration granularity")
{
    GIVEN("A range of data lines to migrate")
    {
        WHEN("Its migration granularity is calculated")
        {
            THEN("The range is covered by the smallest granularity that fits it")
            {
                for (unsigned distance = 0; distance < START_ADDRESS_WIDTH(OS_TRANSPARENT_MANAGEMENT::StartAddress::Max); distance++)
                {
                    const MIGRATION_GRANULARITY_WIDTH granularity = OS_TRANSPARENT_MANAGEMENT::calculate_migration_granularity(0, static_cast<START_ADDRESS_WIDTH>(distance));

                    REQUIRE(granularity == next_power_of_two_above(distance));
                    REQUIRE(granularity > distance);
                }
            }
        }

        WHEN("The range does not start at the first data line")
        {
            THEN("Only its length decides the granularity")
            {
                REQUIRE(OS_TRANSPARENT_MANAGEMENT::calculate_migration_granularity(8, 12) == OS_TRANSPARENT_MANAGEMENT::calculate_migration_granularity(0, 4));
                REQUIRE(OS_TRANSPARENT_MANAGEMENT::calculate_migration_granularity(31, 31) == OS_TRANSPARENT_MANAGEMENT::calculate_migration_granularity(0, 0));
            }
        }
    }
}

/**
 * Both callers below shrink an oversized migration granularity; they differ only in what they
 * must stay inside. adjust_migration_granularity() keeps the migration within the 4 KiB page,
 * round_down_migration_granularity() keeps it within the accessed range.
 */
SCENARIO("Variable granularity shrinks a migration that runs past the end of its page")
{
    GIVEN("A migration granularity that overruns the page")
    {
        WHEN("It is adjusted")
        {
            THEN("The migration ends inside the page and matches the granularity it was given")
            {
                for (START_ADDRESS_WIDTH start_address = 0; start_address < START_ADDRESS_WIDTH(OS_TRANSPARENT_MANAGEMENT::StartAddress::Max); start_address++)
                {
                    MIGRATION_GRANULARITY_WIDTH granularity       = MIGRATION_GRANULARITY_WIDTH(OS_TRANSPARENT_MANAGEMENT::MigrationGranularity::KiB_4);
                    const START_ADDRESS_WIDTH updated_end_address = OS_TRANSPARENT_MANAGEMENT::adjust_migration_granularity(start_address, last_data_line, granularity);

                    REQUIRE(updated_end_address <= last_data_line);
                    REQUIRE(updated_end_address == start_address + granularity - 1);
                }
            }
        }

        WHEN("A migration granularity already inside the page is adjusted")
        {
            MIGRATION_GRANULARITY_WIDTH granularity       = MIGRATION_GRANULARITY_WIDTH(OS_TRANSPARENT_MANAGEMENT::MigrationGranularity::Byte_256);
            const START_ADDRESS_WIDTH updated_end_address = OS_TRANSPARENT_MANAGEMENT::adjust_migration_granularity(0, last_data_line, granularity);

            THEN("It is left alone")
            {
                REQUIRE(granularity == MIGRATION_GRANULARITY_WIDTH(OS_TRANSPARENT_MANAGEMENT::MigrationGranularity::Byte_256));
                REQUIRE(updated_end_address == MIGRATION_GRANULARITY_WIDTH(OS_TRANSPARENT_MANAGEMENT::MigrationGranularity::Byte_256) - 1);
            }
        }
    }
}

SCENARIO("Variable granularity shrinks a migration that runs past the data it needs")
{
    GIVEN("A migration granularity larger than the accessed range")
    {
        WHEN("It is rounded down")
        {
            THEN("The migration ends inside the accessed range and matches the granularity it was given")
            {
                for (START_ADDRESS_WIDTH end_address = 0; end_address < START_ADDRESS_WIDTH(OS_TRANSPARENT_MANAGEMENT::StartAddress::Max); end_address++)
                {
                    MIGRATION_GRANULARITY_WIDTH granularity       = MIGRATION_GRANULARITY_WIDTH(OS_TRANSPARENT_MANAGEMENT::MigrationGranularity::KiB_4);
                    const START_ADDRESS_WIDTH updated_end_address = OS_TRANSPARENT_MANAGEMENT::round_down_migration_granularity(0, end_address, granularity);

                    REQUIRE(updated_end_address <= end_address);
                    REQUIRE(updated_end_address == granularity - 1);
                }
            }
        }
    }

    GIVEN("An accessed range that reaches the end of the page")
    {
        WHEN("The same oversized granularity is adjusted and rounded down")
        {
            MIGRATION_GRANULARITY_WIDTH adjusted_granularity   = MIGRATION_GRANULARITY_WIDTH(OS_TRANSPARENT_MANAGEMENT::MigrationGranularity::KiB_4);
            MIGRATION_GRANULARITY_WIDTH rounded_granularity    = MIGRATION_GRANULARITY_WIDTH(OS_TRANSPARENT_MANAGEMENT::MigrationGranularity::KiB_4);

            const START_ADDRESS_WIDTH adjusted_end_address     = OS_TRANSPARENT_MANAGEMENT::adjust_migration_granularity(4, last_data_line, adjusted_granularity);
            const START_ADDRESS_WIDTH rounded_down_end_address = OS_TRANSPARENT_MANAGEMENT::round_down_migration_granularity(4, last_data_line, rounded_granularity);

            THEN("They agree, because the page end and the range end are the same limit")
            {
                REQUIRE(adjusted_granularity == rounded_granularity);
                REQUIRE(adjusted_end_address == rounded_down_end_address);
            }
        }
    }
}

#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT, IDEAL_VARIABLE_GRANULARITY */
