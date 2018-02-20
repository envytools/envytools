cdef extern from "nvhw/chipset.h":
    struct chipset_info:
        unsigned pmc_id
        int chipset
        int card_type
        int endian

cdef extern from "nva.h":
    struct nva_card:
        chipset_info chipset
        void *bar0
        void *bar1
        void *bar2
        unsigned bar0len
        unsigned bar1len
        unsigned bar2len
        int hasbar1
        int hasbar2
    nva_card **nva_cards
    int nva_cardsnum
    int nva_init()
    unsigned nva_grd32(void *base, unsigned addr)
    void nva_gwr32(void *base, unsigned addr, unsigned val)
    unsigned nva_grd8(void *base, unsigned addr)
    void nva_gwr8(void *base, unsigned addr, unsigned val)

cdef class NvaBar:
    cdef void *addr
    cdef unsigned len
    def __cinit__(self):
        self.addr = NULL
        self.len = 0
    def rd32(self, addr):
        assert addr + 4 <= self.len
        return nva_grd32(self.addr, addr)
    def wr32(self, addr, val):
        assert addr + 4 <= self.len
        nva_gwr32(self.addr, addr, val)
    def rd8(self, addr):
        assert addr + 1 <= self.len
        return nva_grd8(self.addr, addr)
    def wr8(self, addr, val):
        assert addr + 1 <= self.len
        nva_gwr8(self.addr, addr, val)
    def __len__(self):
        return self.len

cdef NvaBar nva_wrapbar(void *addr, unsigned len_):
    cdef NvaBar res = NvaBar()
    res.addr = addr
    res.len = len_
    return res

cdef class NvaCard_:
    cdef nva_card *card
    def __cinit__(self):
        self.card = NULL

class NvaCard(NvaCard_):
    pass

cdef NvaCard_ nva_wrapcard(nva_card *ccard):
    cdef NvaCard_ card
    card = NvaCard()
    card.card = ccard
    card.bar0 = nva_wrapbar(ccard.bar0, ccard.bar0len)
    card.bar1 = nva_wrapbar(ccard.bar1, ccard.bar1len) if ccard.hasbar1 else None
    card.bar2 = nva_wrapbar(ccard.bar2, ccard.bar2len) if ccard.hasbar2 else None
    # XXX clean up?
    card.chipset = ccard.chipset.chipset
    card.pmc_id = ccard.chipset.pmc_id
    card.card_type = ccard.chipset.card_type
    card.endian = ccard.chipset.endian
    return card

if nva_init():
    raise Exception('nva init failed')

cards = []
for i in range(nva_cardsnum):
    cards.append(nva_wrapcard(nva_cards[i]))
