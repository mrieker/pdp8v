
// kernel module for raspi to map the 1-MHz counter page (read only)
// sets it up as a mmap page accessed from /proc/armmhz

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");

#define MHZ_PROCNAME "armmhz"
#define MHZ_PHYSADDR 0x20003000

static int mhz_open (struct inode *inode, struct file *filp);
static ssize_t mhz_read (struct file *filp, char __user *buff, size_t size, loff_t *posn);
static int mhz_release (struct inode *inode, struct file *filp);
static int mhz_mmap (struct file *filp, struct vm_area_struct *vma);

static struct proc_ops const mhz_fops = {
    .proc_mmap    = mhz_mmap,
    .proc_open    = mhz_open,
    .proc_read    = mhz_read,
    .proc_release = mhz_release
};

int init_module ()
{
    struct proc_dir_entry *procDirEntry = proc_create (MHZ_PROCNAME, S_IRUSR | S_IRGRP | S_IROTH, NULL, &mhz_fops);
    if (procDirEntry == NULL) {
        printk ("armmhz: error creating /proc/%s entry\n", MHZ_PROCNAME);
        return -1;
    }
    return 0;
}

void cleanup_module ()
{
    remove_proc_entry (MHZ_PROCNAME, NULL);
}

static int mhz_open (struct inode *inode, struct file *filp)
{
    if (filp->f_flags != O_RDONLY) return -EACCES;
    return 0;
}

static ssize_t mhz_read (struct file *filp, char __user *buff, size_t size, loff_t *posn)
{
    return -EIO;
}

static int mhz_release (struct inode *inode, struct file *filp)
{
    return 0;
}

static int mhz_mmap (struct file *filp, struct vm_area_struct *vma)
{
    vma->vm_page_prot = pgprot_noncached (vma->vm_page_prot);
    return vm_iomap_memory (vma, MHZ_PHYSADDR, 4096);
}
