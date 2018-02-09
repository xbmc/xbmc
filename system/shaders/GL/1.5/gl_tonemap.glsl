float tonemap(float val)
{
  return val * (1 + val/(m_toneP1*m_toneP1))/(1 + val);
}
