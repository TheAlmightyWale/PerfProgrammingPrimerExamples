class Foo1 {
public:
	int Bar(int count)
	{
		int value = 0;
		for (int i = 0; i < count; i++)
		{
			if (m_bNeedParentUpdate)
			{
				value++;
			}
		}
		return value;
	}


	int Baz(int count)
	{
		int value = 0;
		for (int i = 0; i < count; i++)
		{
			if (Bar(count) > 0)
			{
				value++;
			}
		}

		return value;
	}

	bool m_bNeedParentUpdate = true;
};

class Foo2 {
public:
	int Bar(int count)
	{
		int value = 0;
		bool needUpdate = m_bNeedParentUpdate;
		for (int i = 0; i < count; i++)
		{
			if (needUpdate)
			{
				value++;
			}
		}
		return value;
	}


	int Baz(int count)
	{
		int value = 0;
		bool needUpdate = Bar(count) > 0;
		for (int i = 0; i < count; i++)
		{
			if (needUpdate)
			{
				value++;
			}
		}

		return value;
	}

	bool m_bNeedParentUpdate = true;
};

class Foo3
{
public:
	int Bar(int count)
	{
		int value = 0;
		bool needUpdate = m_bNeedParentUpdate;
		if (needUpdate)
		{
			for (int i = 0; i < count; i++)
			{
				if (needUpdate)
				{
					value++;
				}
			}
		}
		return value;
	}


	int Baz(int count)
	{
		int value = 0;
		bool needUpdate = Bar(count) > 0;
		if (needUpdate)
		{
			for (int i = 0; i < count; i++)
			{

				value++;
			}
		}

		return value;
	}

	bool m_bNeedParentUpdate = true;
};