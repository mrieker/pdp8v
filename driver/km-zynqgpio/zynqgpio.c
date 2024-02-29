
// kernel module for raspi to map the 1-MHz counter page (read only)
// sets it up as a mmap page accessed from /proc/zynqgpio

#include <linux/version.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

#define ZG_PROCNAME "zynqgpio"
#define ZG_PHYSADDR 0x43C00000

typedef struct FoCtx {
    unsigned long pagepa;
} FoCtx;

static int zg_open (struct inode *inode, struct file *filp);
static ssize_t zg_read (struct file *filp, char __user *buff, size_t size, loff_t *posn);
static ssize_t zg_write (struct file *filp, char const __user *buff, size_t size, loff_t *posn);
static int zg_release (struct inode *inode, struct file *filp);
static long zg_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);
static int zg_mmap (struct file *filp, struct vm_area_struct *vma);


static struct file_operations const zg_fops = {
    .mmap    = zg_mmap,
    .open    = zg_open,
    .read    = zg_read,
    .release = zg_release,
    .unlocked_ioctl = zg_ioctl,
    .write   = zg_write
};

int init_module ()
{
    struct proc_dir_entry *procDirEntry = proc_create (ZG_PROCNAME, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH, NULL, &zg_fops);
    if (procDirEntry == NULL) {
        printk ("zynqgpio: error creating /proc/%s entry\n", ZG_PROCNAME);
        return -1;
    }
    return 0;
}

void cleanup_module ()
{
    remove_proc_entry (ZG_PROCNAME, NULL);
}

static int zg_open (struct inode *inode, struct file *filp)
{
    FoCtx *foctx;

    filp->private_data = NULL;
    if (filp->f_flags != O_RDWR) return -EACCES;
    foctx = kmalloc (sizeof *foctx, GFP_KERNEL);
    if (foctx == NULL) return -ENOMEM;
    memset (foctx, 0, sizeof *foctx);
    filp->private_data = foctx;
    return 0;
}

static ssize_t zg_read (struct file *filp, char __user *buff, size_t size, loff_t *posn)
{
    return -EIO;
}

static ssize_t zg_write (struct file *filp, char const __user *buff, size_t size, loff_t *posn)
{
    return -EIO;
}

static int zg_release (struct inode *inode, struct file *filp)
{
    FoCtx *foctx = filp->private_data;
    if (foctx != NULL) {
        filp->private_data = NULL;
        kfree (foctx);
    }
    return 0;
}

static long zg_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
    FoCtx *foctx = filp->private_data;
    if (cmd == 13423) {
        if (copy_to_user ((void *) arg, &foctx->pagepa, sizeof foctx->pagepa)) return -EFAULT;
        return foctx->pagepa;
    }
    return -EINVAL;
}

static int zg_mmap (struct file *filp, struct vm_area_struct *vma)
{
    FoCtx *foctx = filp->private_data;
    if (vma->vm_end - vma->vm_start != PAGE_SIZE) {
        printk ("zg_map: not mapping single page, va %lX..%lX\n", vma->vm_start, vma->vm_end);
        return -EINVAL;
    }
    vma->vm_page_prot = pgprot_noncached (vma->vm_page_prot);
    switch (vma->vm_pgoff) {
        case 0: return vm_iomap_memory (vma, ZG_PHYSADDR, PAGE_SIZE);
        case 1: {
            int rc;
            struct page *pages;

            // disallow copy-on-write (must be shared or read-only)
            // lest vm_insert_page() puque
            if ((vma->vm_flags & (VM_SHARED | VM_MAYWRITE)) == VM_MAYWRITE) {
                printk("zg_mmap: mmap() is_cow_mapping() true\n");
                return -EINVAL;
            }

            // allocate a single page
            pages = alloc_pages (GFP_HIGHUSER | __GFP_COMP, 0);
            printk ("zg_mmap*: pages=%p\n", pages);
            if (pages == NULL) {
                printk ("zg_mmap: no page available\n");
                return -ENOMEM;
            }

            // map it to user address
            rc = vm_insert_page (vma, vma->vm_start, pages);
            printk ("zg_mmap*: rc=%d va=%lX\n", rc, vma->vm_start);
            if (rc < 0) {
                printk ("zg_map: error %d mapping page\n", rc);
                return rc;
            }

            // save physical address in context
            foctx->pagepa = (unsigned long) page_to_pfn (pages) * PAGE_SIZE;
            printk ("zg_mmap*: pa=%lX\n", foctx->pagepa);
            return 0;
        }
        default: {
            printk ("zg_map: bad file offset %lX\n", vma->vm_pgoff);
            return -EINVAL;
        }
    }
}
