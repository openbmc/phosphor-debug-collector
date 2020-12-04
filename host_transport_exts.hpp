namespace phosphor
{
namespace dump
{
namespace host
{

/**
 * @brief Initiate offload of the dump with provided id
 *
 * @param[in] id - The Dump Source ID.
 *
 */
void requestOffload(uint32_t id);

/**
 * @brief Request to delete dump
 *
 * @param[in] id - The Dump Source ID.
 * @return NULL
 *
 */
void requestDelete(uint32_t id);

} // namespace host
} // namespace dump
} // namespace phosphor
