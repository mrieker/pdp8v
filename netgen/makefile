
CLASSDIR = classes

MACH = $(shell uname -m)

JAVAC = javac -Xlint:unchecked -d $(CLASSDIR) -sourcepath .

CP = $(CLASSDIR):jacl141/lib/tcljava1.4.1/tcljava.jar:jacl141/lib/tcljava1.4.1/jacl.jar

CLASSFILES = \
	BufLEDModule.class \
	Comp.class \
	CompTree.class \
	ConnModule.class \
	Const.class \
	DAOModule.class \
	DFFModule.class \
	DLatModule.class \
	Gate.class \
	GenCtx.class \
	HaltedException.class \
	ICombo.class \
	IParam.class \
	IPrintLoc.class \
	ISequen.class \
	IVerilog.class \
	LabelModule.class \
	MapEdit.class \
	Module.class \
	NetClass.class \
	NetGen.class \
	Network.class \
	OParam.class \
	OpndLhs.class \
	OpndRhs.class \
	ParsePCB.class \
	Placeable.class \
	PropNode.class \
	PropPrinter.class \
	PropRoot.class \
	PulseModule.class \
	PwrConnModule.class \
	RasPiModule.class \
	Reduce.class \
	ReduceEx.class \
	RField.class \
	Simulator.class \
	StandCell.class \
	Token.class \
	UserModule.class \
	WField.class \
	WiredAnd.class

default: $(addprefix $(CLASSDIR)/,$(CLASSFILES))

clean:
	rm -rf $(CLASSDIR)

$(CLASSDIR)/%.class: %.java jacl141/lib/tcljava1.4.1/tcljava.jar
	mkdir -p $(CLASSDIR)
	rm -f $@ $(@:.class=\$$*.class)
	CLASSPATH=$(CP) $(JAVAC) $<

jacl141/lib/tcljava1.4.1/tcljava.jar: jaclBinary141.zip
	unzip jaclBinary141.zip
	touch jacl141/lib/tcljava1.4.1/*.jar

jaclBinary141.zip:
	wget --no-check-certificate https://phoenixnap.dl.sourceforge.net/project/tcljava/jacl/1.4.1/jaclBinary141.zip

