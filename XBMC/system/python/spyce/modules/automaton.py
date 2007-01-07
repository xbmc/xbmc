##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

from spyceModule import spyceModule

__doc__ = '''Automaton module allows Spyce users to create websites with
state machine-based application flows. One can define an automaton
programmatically using the start(), transition() and begin methods. The
automaton is the executed (one step per request) using the step() method. This
method accepts the current state, which should be managed by the user
preferably via a session (keeping the information on the server), or possibly
by get, post or cookie. The step() method then calls the recv() function on
the given state, which returns an edge label. This edge points to the new
state. The step() method then calls the send() method of the new state to
generate the page content. The user should encode the new state in this
content, or use on a subsequent request.'''

SEND = 0
RECV = 1
EDGES = 2

class automaton(spyceModule):
  def start(self):
    "Initialise an empty automaton"
    self.clear()
  def clear(self):
    self._nodes = {}
    self._edges = {}
  # defining the automaton
  def state(self, name, send, recv):
    "Add a new automaton state"
    self._nodes[name] = send, recv
    self.transition(name, None, name)
  def transition(self, state1, edge, state2):
    "Add a new automaton transition"
    if not self._nodes.has_key(state1):
      raise 'state %s does not exist' % state1
    if not self._nodes.has_key(state2):
      raise 'state %s does not exist' % state2
    self._edges[(state1, edge)] = state2
  node=state
  edge=transition
  def begin(self, name):
    if not self._nodes.has_key(name):
      raise 'state %s does not exist' % name
    self._begin = name
  def define(self, sm, start):
    self.clear()
    for s1 in sm.keys():
      self.node(s1, sm[s1][SEND], sm[s1][RECV])
    for s1 in sm.keys():
      for e in sm[s1][EDGES].keys():
        self.edge(s1, e, sm[s1][EDGES][e])
    self.begin(start)

  # running the automaton
  def step(self, state=None):
    """Run the automaton one step: recv (old state), transition, 
    send (new state)"""
    if state==None:
      state = self._begin
    else:
      try: _, recv = self._nodes[state]
      except: raise 'invalid state: %s' % state
      edge = recv()
      try: state = self._edges[(state, edge)]
      except: raise 'invalid transition: %s,%s' % (state, edge)
    try: send, _ = self._nodes[state]
    except: raise 'invalid state: %s' % state
    send()


# rimtodo: cached state-machines

