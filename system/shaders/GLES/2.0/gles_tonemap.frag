float tonemap(float val)
{
  return val * (1.0 + val / (m_toneP1 * m_toneP1)) / (1.0 + val);
}
