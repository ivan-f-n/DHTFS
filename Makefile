PROG=dhtfs
OBJDIR=.obj
CC=g++

CFLAGS = -Wall --std=c++14 -lopendht -lgnutls `pkg-config fuse3 --cflags ` -I.. -I/home/msgpack-c/include 
LDFLAGS = -lopendht -lgnutls `pkg-config fuse3 --libs `

$(shell mkdir -p $(OBJDIR)) 

OBJS = $(OBJDIR)/P2PFS.o $(OBJDIR)/FSpart.o

$(PROG) : $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(PROG)

-include $(OBJS:.o=.d)

$(OBJDIR)/%.o: src/%.cpp
	$(CC) -c $(CFLAGS) src/$*.cpp -o $(OBJDIR)/$*.o
	$(CC) -MM $(CFLAGS) src/$*.cpp > $(OBJDIR)/$*.d
	@mv -f $(OBJDIR)/$*.d $(OBJDIR)/$*.d.tmp
	@sed -e 's|.*:|$(OBJDIR)/$*.o:|' < $(OBJDIR)/$*.d.tmp > $(OBJDIR)/$*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $(OBJDIR)/$*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $(OBJDIR)/$*.d
	@rm -f $(OBJDIR)/$*.d.tmp

clean:
	rm -rf $(PROG) $(OBJDIR)

