#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __LINUX_XHCI_MVEBU_H
#define __LINUX_XHCI_MVEBU_H

#if defined(CONFIG_USB_XHCI_MVEBU) || defined(MY_ABC_HERE)
int xhci_mvebu_probe(struct platform_device *pdev);
int xhci_mvebu_remove(struct platform_device *pdev);
void xhci_mvebu_resume(struct device *dev);
#else
#define xhci_mvebu_probe NULL
#define xhci_mvebu_remove NULL
#define xhci_mvebu_resume NULL
#endif
#endif  
