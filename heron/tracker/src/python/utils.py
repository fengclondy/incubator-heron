"""
Contains utility functions used by tracker.
"""

import string

def hex_escape(bin_str):
  """
  Hex encode a binary string
  """
  printable = string.ascii_letters + string.digits + string.punctuation + ' '
  return ''.join(ch if ch in printable else r'0x{0:02x}'.format(ord(ch)) for ch in bin_str)

def make_shell_endpoint(topologyInfo, instance_id):
  """
  Makes the http endpoint for the heron shell
  if shell port is present, otherwise returns None.
  """
  # Format: container_<id>_<instance_id>
  component_id = instance_id.split('_')[1]
  pplan = topologyInfo["physical_plan"]
  stmgrId = pplan["instances"][instance_id]["stmgrId"]
  host = pplan["stmgrs"][stmgrId]["host"]
  shell_port = pplan["stmgrs"][stmgrId]["shell_port"]
  return "http://%s:%d" % (host, shell_port)

def make_shell_job_url(host, shell_port, cwd):
  """
  Make the job url from the info
  stored in stmgr. This points to dir from where
  all the processes are started.
  If shell port is not present, it returns the url
  as generated by aurora.
  """
  return "http://%s:%d/browse/" % (host, shell_port)

def make_shell_logfiles_url(host, shell_port, cwd, instance_id=None):
  """
  Make the url for log-files in heron-shell
  from the info stored in stmgr.
  If no instance_id is provided, the link will
  be to the dir for the whole container.
  If shell port is not present, it returns the url
  as generated by aurora.
  """
  if not instance_id:
    return "http://%s:%d/browse/log-files" % (host, shell_port)
  else:
    return "http://%s:%d/file/log-files/%s.log.0" % (host, shell_port, instance_id)

def make_viz_dashboard_url(name, dc, environ):
  """
  Link to the dashboard. Must override to return a valid url.
  """
  return ""

def convert_execution_state(execution_state):
  """
  For backward compatibility. Right now return as it is.
  """
  return execution_state

