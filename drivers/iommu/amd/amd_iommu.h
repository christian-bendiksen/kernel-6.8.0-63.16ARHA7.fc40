/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2009-2010 Advanced Micro Devices, Inc.
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#ifndef AMD_IOMMU_H
#define AMD_IOMMU_H

#include <linux/iommu.h>

#include "amd_iommu_types.h"

irqreturn_t amd_iommu_int_thread(int irq, void *data);
irqreturn_t amd_iommu_int_thread_evtlog(int irq, void *data);
irqreturn_t amd_iommu_int_thread_pprlog(int irq, void *data);
irqreturn_t amd_iommu_int_thread_galog(int irq, void *data);
irqreturn_t amd_iommu_int_handler(int irq, void *data);
void amd_iommu_apply_erratum_63(struct amd_iommu *iommu, u16 devid);
void amd_iommu_restart_event_logging(struct amd_iommu *iommu);
void amd_iommu_restart_ga_log(struct amd_iommu *iommu);
void amd_iommu_restart_ppr_log(struct amd_iommu *iommu);
void amd_iommu_set_rlookup_table(struct amd_iommu *iommu, u16 devid);

#ifdef CONFIG_AMD_IOMMU_DEBUGFS
void amd_iommu_debugfs_setup(struct amd_iommu *iommu);
#else
static inline void amd_iommu_debugfs_setup(struct amd_iommu *iommu) {}
#endif

/* Needed for interrupt remapping */
int amd_iommu_prepare(void);
int amd_iommu_enable(void);
void amd_iommu_disable(void);
int amd_iommu_reenable(int mode);
int amd_iommu_enable_faulting(void);
extern int amd_iommu_guest_ir;
extern enum io_pgtable_fmt amd_iommu_pgtable;
extern int amd_iommu_gpt_level;

bool amd_iommu_v2_supported(void);

/* Device capabilities */
int amd_iommu_pdev_enable_cap_pri(struct pci_dev *pdev);
void amd_iommu_pdev_disable_cap_pri(struct pci_dev *pdev);

/* GCR3 setup */
int amd_iommu_set_gcr3(struct iommu_dev_data *dev_data,
		       ioasid_t pasid, unsigned long gcr3);
int amd_iommu_clear_gcr3(struct iommu_dev_data *dev_data, ioasid_t pasid);

/*
 * This function flushes all internal caches of
 * the IOMMU used by this driver.
 */
void amd_iommu_flush_all_caches(struct amd_iommu *iommu);
void amd_iommu_update_and_flush_device_table(struct protection_domain *domain);
void amd_iommu_domain_update(struct protection_domain *domain);
void amd_iommu_domain_flush_complete(struct protection_domain *domain);
void amd_iommu_domain_flush_pages(struct protection_domain *domain,
				  u64 address, size_t size);
void amd_iommu_dev_flush_pasid_pages(struct iommu_dev_data *dev_data,
				     ioasid_t pasid, u64 address, size_t size);
void amd_iommu_dev_flush_pasid_all(struct iommu_dev_data *dev_data,
				   ioasid_t pasid);

#ifdef CONFIG_IRQ_REMAP
int amd_iommu_create_irq_domain(struct amd_iommu *iommu);
#else
static inline int amd_iommu_create_irq_domain(struct amd_iommu *iommu)
{
	return 0;
}
#endif

int amd_iommu_complete_ppr(struct pci_dev *pdev, u32 pasid,
			   int status, int tag);

static inline bool is_rd890_iommu(struct pci_dev *pdev)
{
	return (pdev->vendor == PCI_VENDOR_ID_ATI) &&
	       (pdev->device == PCI_DEVICE_ID_RD890_IOMMU);
}

static inline bool check_feature(u64 mask)
{
	return (amd_iommu_efr & mask);
}

static inline bool check_feature2(u64 mask)
{
	return (amd_iommu_efr2 & mask);
}

static inline int check_feature_gpt_level(void)
{
	return ((amd_iommu_efr >> FEATURE_GATS_SHIFT) & FEATURE_GATS_MASK);
}

static inline bool amd_iommu_gt_ppr_supported(void)
{
	return (check_feature(FEATURE_GT) &&
		check_feature(FEATURE_PPR));
}

static inline u64 iommu_virt_to_phys(void *vaddr)
{
	return (u64)__sme_set(virt_to_phys(vaddr));
}

static inline void *iommu_phys_to_virt(unsigned long paddr)
{
	return phys_to_virt(__sme_clr(paddr));
}

static inline
void amd_iommu_domain_set_pt_root(struct protection_domain *domain, u64 root)
{
	domain->iop.root = (u64 *)(root & PAGE_MASK);
	domain->iop.mode = root & 7; /* lowest 3 bits encode pgtable mode */
}

static inline
void amd_iommu_domain_clr_pt_root(struct protection_domain *domain)
{
	amd_iommu_domain_set_pt_root(domain, 0);
}

static inline int get_pci_sbdf_id(struct pci_dev *pdev)
{
	int seg = pci_domain_nr(pdev->bus);
	u16 devid = pci_dev_id(pdev);

	return PCI_SEG_DEVID_TO_SBDF(seg, devid);
}

static inline void *alloc_pgtable_page(int nid, gfp_t gfp)
{
	struct page *page;

	page = alloc_pages_node(nid, gfp | __GFP_ZERO, 0);
	return page ? page_address(page) : NULL;
}

/*
 * This must be called after device probe completes. During probe
 * use rlookup_amd_iommu() get the iommu.
 */
static inline struct amd_iommu *get_amd_iommu_from_dev(struct device *dev)
{
	return iommu_get_iommu_dev(dev, struct amd_iommu, iommu);
}

/* This must be called after device probe completes. */
static inline struct amd_iommu *get_amd_iommu_from_dev_data(struct iommu_dev_data *dev_data)
{
	return iommu_get_iommu_dev(dev_data->dev, struct amd_iommu, iommu);
}

bool translation_pre_enabled(struct amd_iommu *iommu);
bool amd_iommu_is_attach_deferred(struct device *dev);
int __init add_special_device(u8 type, u8 id, u32 *devid, bool cmd_line);

#ifdef CONFIG_DMI
void amd_iommu_apply_ivrs_quirks(void);
#else
static inline void amd_iommu_apply_ivrs_quirks(void) { }
#endif

void amd_iommu_domain_set_pgtable(struct protection_domain *domain,
				  u64 *root, int mode);
struct dev_table_entry *get_dev_table(struct amd_iommu *iommu);

#endif
