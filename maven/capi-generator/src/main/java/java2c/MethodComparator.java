package java2c;

import java.util.Comparator;
import java.lang.reflect.Method;

public class MethodComparator implements Comparator<Method> {

    private final CAPIGenerator capiGenerator;

    MethodComparator(CAPIGenerator capiGenerator) {
        this.capiGenerator = capiGenerator;
    }

    public int compare(Method method1, Method method2) {
        if (method1 == null && method2 == null) {
            return 0;
        }
        if (method1 == null) {
            return -1;
        }
        if (method2 == null) {
            return 1;
        }

        String sig1 = capiGenerator.getMethodName(method1.getName(), method1.getParameterTypes());
        String sig2 = capiGenerator.getMethodName(method2.getName(), method2.getParameterTypes());

        return sig1.compareTo(sig2);
    }
}
