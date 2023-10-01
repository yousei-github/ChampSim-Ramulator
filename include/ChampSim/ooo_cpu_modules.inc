enum class bpred_t
{
    bbranchDbimodal
};

void bpred_bbranchDbimodal_initialize();
void impl_branch_predictor_initialize()
{
    if (bpred_type == bpred_t::bbranchDbimodal) return bpred_bbranchDbimodal_initialize();
    throw std::invalid_argument("Branch predictor module not found");
}

void bpred_bbranchDbimodal_last_result(uint64_t, uint64_t, uint8_t, uint8_t);
void impl_last_branch_result(uint64_t ip, uint64_t target, uint8_t taken, uint8_t branch_type)
{
    if (bpred_type == bpred_t::bbranchDbimodal) return bpred_bbranchDbimodal_last_result(ip, target, taken, branch_type);
    throw std::invalid_argument("Branch predictor module not found");
}

uint8_t bpred_bbranchDbimodal_predict(uint64_t, uint64_t, uint8_t, uint8_t);
uint8_t impl_predict_branch(uint64_t ip, uint64_t predicted_target, uint8_t always_taken, uint8_t branch_type)
{
    if (bpred_type == bpred_t::bbranchDbimodal) return bpred_bbranchDbimodal_predict(ip, predicted_target, always_taken, branch_type);
    throw std::invalid_argument("Branch predictor module not found");
    return 0;
}

enum class btb_t
{
    bbtbDbasic_btb
};

void btb_bbtbDbasic_btb_initialize();
void impl_btb_initialize()
{
    if (btb_type == btb_t::bbtbDbasic_btb) return btb_bbtbDbasic_btb_initialize();
    throw std::invalid_argument("Branch target buffer module not found");
}

void btb_bbtbDbasic_btb_update(uint64_t, uint64_t, uint8_t, uint8_t);
void impl_update_btb(uint64_t ip, uint64_t branch_target, uint8_t taken, uint8_t branch_type)
{
    if (btb_type == btb_t::bbtbDbasic_btb) return btb_bbtbDbasic_btb_update(ip, branch_target, taken, branch_type);
    throw std::invalid_argument("Branch target buffer module not found");
}

std::pair<uint64_t, uint8_t> btb_bbtbDbasic_btb_predict(uint64_t, uint8_t);
std::pair<uint64_t, uint8_t> impl_btb_prediction(uint64_t ip, uint8_t branch_type)
{
    if (btb_type == btb_t::bbtbDbasic_btb) return btb_bbtbDbasic_btb_predict(ip, branch_type);
    throw std::invalid_argument("Branch target buffer module not found");
}

enum class ipref_t
{
    pprefetcherDno_instr
};

void pref_pprefetcherDno_instr_initialize();
void pref_pprefetcherDno_instr_branch_operate(uint64_t, uint8_t, uint64_t);
void impl_prefetcher_branch_operate(uint64_t ip, uint8_t branch_type, uint64_t branch_target)
{
    if (ipref_type == ipref_t::pprefetcherDno_instr) return pref_pprefetcherDno_instr_branch_operate(ip, branch_type, branch_target);
    throw std::invalid_argument("Instruction prefetcher module not found");
}

uint32_t pref_pprefetcherDno_instr_cache_operate(uint64_t, uint8_t, uint8_t, uint32_t);
void pref_pprefetcherDno_instr_cycle_operate();
void impl_prefetcher_cycle_operate()
{
    if (ipref_type == ipref_t::pprefetcherDno_instr) return pref_pprefetcherDno_instr_cycle_operate();
    throw std::invalid_argument("Instruction prefetcher module not found");
}

uint32_t pref_pprefetcherDno_instr_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);
void pref_pprefetcherDno_instr_final_stats();
