
from PyObjCTools.TestSupport import *
from WebKit import *

class TestDOMHTMLTableCellElement (TestCase):
    def testMethods(self):
        self.failUnlessResultIsBOOL(DOMHTMLTableCellElement.noWrap)
        self.failUnlessArgIsBOOL(DOMHTMLTableCellElement.setNoWrap_, 0)

if __name__ == "__main__":
    main()
